/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details:
 *
 * Copyright (C) 2014 Ammonit Measurement GmbH
 * Copyright (C) 2014 - 2018 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>

#include <glib-object.h>
#include <gio/gio.h>

#define _LIBMM_INSIDE_MM
#include <libmm-glib.h>

#include "mm-log.h"
#include "mm-iface-modem.h"
#include "mm-iface-modem-location.h"
#include "mm-base-modem.h"
#include "mm-base-modem-at.h"
#include "mm-shared-cinterion.h"

/*****************************************************************************/
/* Private data context */

#define PRIVATE_TAG "shared-cinterion-private-tag"
static GQuark private_quark;

typedef enum {
    FEATURE_SUPPORT_UNKNOWN,
    FEATURE_NOT_SUPPORTED,
    FEATURE_SUPPORTED,
} FeatureSupport;

typedef struct {
    MMIfaceModemLocation  *iface_modem_location_parent;
    MMModemLocationSource  supported_sources;
    MMModemLocationSource  enabled_sources;
    FeatureSupport         sgpss_support;
    FeatureSupport         sgpsc_support;
} Private;

static void
private_free (Private *ctx)
{
    g_slice_free (Private, ctx);
}

static Private *
get_private (MMSharedCinterion *self)
{
    Private *priv;

    if (G_UNLIKELY (!private_quark))
        private_quark =  (g_quark_from_static_string (PRIVATE_TAG));

    priv = g_object_get_qdata (G_OBJECT (self), private_quark);
    if (!priv) {
        priv = g_slice_new (Private);

        priv->supported_sources = MM_MODEM_LOCATION_SOURCE_NONE;
        priv->enabled_sources = MM_MODEM_LOCATION_SOURCE_NONE;
        priv->sgpss_support = FEATURE_SUPPORT_UNKNOWN;
        priv->sgpsc_support = FEATURE_SUPPORT_UNKNOWN;

        /* Setup parent class' MMIfaceModemLocation */
        g_assert (MM_SHARED_CINTERION_GET_INTERFACE (self)->peek_parent_location_interface);
        priv->iface_modem_location_parent = MM_SHARED_CINTERION_GET_INTERFACE (self)->peek_parent_location_interface (self);

        g_object_set_qdata_full (G_OBJECT (self), private_quark, priv, (GDestroyNotify)private_free);
    }

    return priv;
}

/*****************************************************************************/
/* GPS trace received */

static void
trace_received (MMPortSerialGps *port,
                const gchar *trace,
                MMIfaceModemLocation *self)
{
    /* Helper to debug GPS location related issues. Don't depend on a real GPS
     * fix for debugging, just use some random values to update */
#if 0
    if (g_str_has_prefix (trace, "$GPGGA")) {
        GString *str;
        GDateTime *now;

        now = g_date_time_new_now_utc ();
        str = g_string_new ("");
        g_string_append_printf (str,
                                "$GPGGA,%02u%02u%02u,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
                                g_date_time_get_hour (now),
                                g_date_time_get_minute (now),
                                g_date_time_get_second (now));
        mm_iface_modem_location_gps_update (self, str->str);
        g_string_free (str, TRUE);
        g_date_time_unref (now);
        return;
    }
#endif

    mm_iface_modem_location_gps_update (self, trace);
}

/*****************************************************************************/
/* Location capabilities loading (Location interface) */

MMModemLocationSource
mm_shared_cinterion_location_load_capabilities_finish (MMIfaceModemLocation  *self,
                                                       GAsyncResult          *res,
                                                       GError               **error)
{
    GError *inner_error = NULL;
    gssize aux;

    aux = g_task_propagate_int (G_TASK (res), &inner_error);
    if (inner_error) {
        g_propagate_error (error, inner_error);
        return MM_MODEM_LOCATION_SOURCE_NONE;
    }
    return (MMModemLocationSource) aux;
}

static void probe_gps_features (GTask *task);

static void
sgpsc_test_ready (MMBaseModem  *self,
                  GAsyncResult *res,
                  GTask        *task)
{
    Private *priv;

    priv = get_private (MM_SHARED_CINTERION (self));

    if (!mm_base_modem_at_command_finish (self, res, NULL))
        priv->sgpsc_support = FEATURE_NOT_SUPPORTED;
    else {
        /* ^SGPSC supported! */
        priv->sgpsc_support = FEATURE_SUPPORTED;
        /* It may happen that the modem was started with GPS already enabled, or
         * maybe ModemManager got rebooted and it was left enabled before. We'll
         * make sure that it is disabled when we initialize the modem. */
        mm_base_modem_at_command (MM_BASE_MODEM (self), "AT^SGPSC=\"Engine\",\"0\"",          3, FALSE, NULL, NULL);
        mm_base_modem_at_command (MM_BASE_MODEM (self), "AT^SGPSC=\"Power/Antenna\",\"off\"", 3, FALSE, NULL, NULL);
        mm_base_modem_at_command (MM_BASE_MODEM (self), "AT^SGPSC=\"NMEA/Output\",\"off\"",   3, FALSE, NULL, NULL);
    }

    probe_gps_features (task);
}

static void
sgpss_test_ready (MMBaseModem  *self,
                  GAsyncResult *res,
                  GTask        *task)
{
    Private *priv;

    priv = get_private (MM_SHARED_CINTERION (self));

    if (!mm_base_modem_at_command_finish (self, res, NULL))
        priv->sgpss_support = FEATURE_NOT_SUPPORTED;
    else {
        /* ^SGPSS supported! */
        priv->sgpss_support = FEATURE_SUPPORTED;

        /* Flag ^SGPSC as unsupported, even if it may be supported, so that we
         * only use one set of commands to enable/disable GPS. */
        priv->sgpsc_support = FEATURE_NOT_SUPPORTED;

        /* It may happen that the modem was started with GPS already enabled, or
         * maybe ModemManager got rebooted and it was left enabled before. We'll
         * make sure that it is disabled when we initialize the modem. */
        mm_base_modem_at_command (MM_BASE_MODEM (self), "AT^SGPSS=0", 3, FALSE, NULL, NULL);
    }

    probe_gps_features (task);
}

static void
probe_gps_features (GTask *task)
{
    MMSharedCinterion     *self;
    MMModemLocationSource  sources;
    Private               *priv;

    self = MM_SHARED_CINTERION (g_task_get_source_object (task));
    priv = get_private (self);

    /* Need to check if SGPSS supported... */
    if (priv->sgpss_support == FEATURE_SUPPORT_UNKNOWN) {
        mm_base_modem_at_command (MM_BASE_MODEM (self), "AT^SGPSS=?", 3, TRUE, (GAsyncReadyCallback) sgpss_test_ready, task);
        return;
    }

    /* Need to check if SGPSC supported... */
    if (priv->sgpsc_support == FEATURE_SUPPORT_UNKNOWN) {
        mm_base_modem_at_command (MM_BASE_MODEM (self), "AT^SGPSC=?", 3, TRUE, (GAsyncReadyCallback) sgpsc_test_ready, task);
        return;
    }

    /* All GPS features probed */

    /* Recover parent sources */
    sources = GPOINTER_TO_UINT (g_task_get_task_data (task));

    if (priv->sgpss_support == FEATURE_SUPPORTED || priv->sgpsc_support == FEATURE_SUPPORTED) {
        mm_dbg ("GPS commands supported: GPS capabilities enabled");

        /* We only flag as supported by this implementation those sources not already
         * supported by the parent implementation */
        if (!(sources & MM_MODEM_LOCATION_SOURCE_GPS_NMEA))
            priv->supported_sources |= MM_MODEM_LOCATION_SOURCE_GPS_NMEA;
        if (!(sources & MM_MODEM_LOCATION_SOURCE_GPS_RAW))
            priv->supported_sources |= MM_MODEM_LOCATION_SOURCE_GPS_RAW;
        if (!(sources & MM_MODEM_LOCATION_SOURCE_GPS_UNMANAGED))
            priv->supported_sources |= MM_MODEM_LOCATION_SOURCE_GPS_UNMANAGED;

        sources |= priv->supported_sources;

        /* Add handler for the NMEA traces in the GPS data port */
        mm_port_serial_gps_add_trace_handler (mm_base_modem_peek_port_gps (MM_BASE_MODEM (self)),
                                              (MMPortSerialGpsTraceFn)trace_received,
                                              self,
                                              NULL);
    } else
        mm_dbg ("No GPS command supported: no GPS capabilities");

    g_task_return_int (task, (gssize) sources);
    g_object_unref (task);
}

static void
parent_load_capabilities_ready (MMIfaceModemLocation *self,
                                GAsyncResult         *res,
                                GTask                *task)
{
    MMModemLocationSource  sources;
    GError                *error = NULL;
    Private               *priv;

    priv = get_private (MM_SHARED_CINTERION (self));

    sources = priv->iface_modem_location_parent->load_capabilities_finish (self, res, &error);
    if (error) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    /* Now our own check. If we don't have any GPS port, we're done */
    if (!mm_base_modem_peek_port_gps (MM_BASE_MODEM (self))) {
        mm_dbg ("No GPS data port found: no GPS capabilities");
        g_task_return_int (task, sources);
        g_object_unref (task);
        return;
    }

    /* Cache sources supported by the parent */
    g_task_set_task_data (task, GUINT_TO_POINTER (sources), NULL);

    /* Probe all GPS features */
    probe_gps_features (task);
}

void
mm_shared_cinterion_location_load_capabilities (MMIfaceModemLocation *self,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data)
{
    Private *priv;
    GTask   *task;

    priv = get_private (MM_SHARED_CINTERION (self));
    task = g_task_new (self, NULL, callback, user_data);

    g_assert (priv->iface_modem_location_parent);
    g_assert (priv->iface_modem_location_parent->load_capabilities);
    g_assert (priv->iface_modem_location_parent->load_capabilities_finish);

    priv->iface_modem_location_parent->load_capabilities (self,
                                                          (GAsyncReadyCallback)parent_load_capabilities_ready,
                                                          task);
}

/*****************************************************************************/
/* Disable location gathering (Location interface) */

typedef enum {
    DISABLE_LOCATION_GATHERING_GPS_STEP_FIRST,
    DISABLE_LOCATION_GATHERING_GPS_STEP_SGPSS,
    DISABLE_LOCATION_GATHERING_GPS_STEP_SGPSC_ENGINE,
    DISABLE_LOCATION_GATHERING_GPS_STEP_SGPSC_ANTENNA,
    DISABLE_LOCATION_GATHERING_GPS_STEP_SGPSC_OUTPUT,
    DISABLE_LOCATION_GATHERING_GPS_STEP_LAST,
} DisableLocationGatheringGpsStep;

typedef struct {
    MMModemLocationSource            source;
    DisableLocationGatheringGpsStep  gps_step;
    GError                          *sgpss_error;
    GError                          *sgpsc_error;
} DisableLocationGatheringContext;

static void
disable_location_gathering_context_free (DisableLocationGatheringContext *ctx)
{
    if (ctx->sgpss_error)
        g_error_free (ctx->sgpss_error);
    if (ctx->sgpsc_error)
        g_error_free (ctx->sgpsc_error);
    g_slice_free (DisableLocationGatheringContext, ctx);
}

gboolean
mm_shared_cinterion_disable_location_gathering_finish (MMIfaceModemLocation  *self,
                                                       GAsyncResult          *res,
                                                       GError               **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void disable_location_gathering_context_gps_step (GTask *task);

static void
disable_sgpsc_ready (MMBaseModem  *self,
                     GAsyncResult *res,
                     GTask        *task)
{
    DisableLocationGatheringContext *ctx;
    GError                          *error = NULL;

    ctx = (DisableLocationGatheringContext *) g_task_get_task_data (task);

    /* Store error, if not one available already, and continue */
    if (!mm_base_modem_at_command_finish (self, res, &error)) {
        if (!ctx->sgpsc_error)
            ctx->sgpsc_error = error;
        else
            g_error_free (error);
    }

    ctx->gps_step++;
    disable_location_gathering_context_gps_step (task);
}

static void
disable_sgpss_ready (MMBaseModem  *self,
                     GAsyncResult *res,
                     GTask        *task)
{
    DisableLocationGatheringContext *ctx;

    ctx = (DisableLocationGatheringContext *) g_task_get_task_data (task);

    /* Store error, if any, and continue */
    g_assert (!ctx->sgpss_error);
    mm_base_modem_at_command_finish (self, res, &ctx->sgpss_error);

    ctx->gps_step++;
    disable_location_gathering_context_gps_step (task);
}

static void
disable_location_gathering_context_gps_step (GTask *task)
{
    DisableLocationGatheringContext *ctx;
    MMSharedCinterion               *self;
    Private                         *priv;

    self = MM_SHARED_CINTERION (g_task_get_source_object (task));
    priv = get_private (self);
    ctx = (DisableLocationGatheringContext *) g_task_get_task_data (task);

    /* Only one of both supported */
    g_assert ((priv->sgpss_support == FEATURE_SUPPORTED) || (priv->sgpsc_support == FEATURE_SUPPORTED));
    g_assert (!((priv->sgpss_support == FEATURE_SUPPORTED) && (priv->sgpsc_support == FEATURE_SUPPORTED)));

    switch (ctx->gps_step) {
    case DISABLE_LOCATION_GATHERING_GPS_STEP_FIRST:
        ctx->gps_step++;
        /* Fall down to next step */

    case DISABLE_LOCATION_GATHERING_GPS_STEP_SGPSS:
        if (priv->sgpss_support == FEATURE_SUPPORTED) {
            mm_base_modem_at_command (MM_BASE_MODEM (self),
                                      "AT^SGPSS=0",
                                      3, FALSE, (GAsyncReadyCallback) disable_sgpss_ready, task);
            return;
        }
        ctx->gps_step++;
        /* Fall down to next step */

    case DISABLE_LOCATION_GATHERING_GPS_STEP_SGPSC_ENGINE:
        if (priv->sgpsc_support == FEATURE_SUPPORTED) {
            /* Engine off */
            mm_base_modem_at_command (MM_BASE_MODEM (self),
                                      "AT^SGPSC=\"Engine\",\"0\"",
                                      3, FALSE, (GAsyncReadyCallback) disable_sgpsc_ready, task);
            return;
        }
        ctx->gps_step++;
        /* Fall down to next step */

    case DISABLE_LOCATION_GATHERING_GPS_STEP_SGPSC_ANTENNA:
        if (priv->sgpsc_support == FEATURE_SUPPORTED) {
            /* Antenna off */
            mm_base_modem_at_command (MM_BASE_MODEM (self),
                                      "AT^SGPSC=\"Power/Antenna\",\"off\"",
                                      3, FALSE, (GAsyncReadyCallback) disable_sgpsc_ready, task);
            return;
        }
        ctx->gps_step++;
        /* Fall down to next step */

    case DISABLE_LOCATION_GATHERING_GPS_STEP_SGPSC_OUTPUT:
        if (priv->sgpsc_support == FEATURE_SUPPORTED) {
            /* NMEA output off */
            mm_base_modem_at_command (MM_BASE_MODEM (self),
                                      "AT^SGPSC=\"NMEA/Output\",\"off\"",
                                      3, FALSE, (GAsyncReadyCallback) disable_sgpsc_ready, task);
            return;
        }
        ctx->gps_step++;
        /* Fall down to next step */

    case DISABLE_LOCATION_GATHERING_GPS_STEP_LAST:
        /* Only use the GPS port in NMEA/RAW setups */
        if (ctx->source & (MM_MODEM_LOCATION_SOURCE_GPS_NMEA | MM_MODEM_LOCATION_SOURCE_GPS_RAW)) {
            MMPortSerialGps *gps_port;

            /* Even if we get an error here, we try to close the GPS port */
            gps_port = mm_base_modem_peek_port_gps (MM_BASE_MODEM (self));
            if (gps_port)
                mm_port_serial_close (MM_PORT_SERIAL (gps_port));
        }

        if (ctx->sgpss_error) {
            g_task_return_error (task, ctx->sgpss_error);
            g_clear_error (&ctx->sgpss_error);
        } else if (ctx->sgpsc_error) {
            g_task_return_error (task, ctx->sgpsc_error);
            g_clear_error (&ctx->sgpsc_error);
        } else {
            priv->enabled_sources &= ~ctx->source;
            g_task_return_boolean (task, TRUE);
        }
        g_object_unref (task);
        return;
    }
}

static void
parent_disable_location_gathering_ready (MMIfaceModemLocation *self,
                                         GAsyncResult         *res,
                                         GTask                *task)
{
    GError  *error;
    Private *priv;

    priv = get_private (MM_SHARED_CINTERION (self));

    g_assert (priv->iface_modem_location_parent);
    if (!priv->iface_modem_location_parent->disable_location_gathering_finish (self, res, &error))
        g_task_return_error (task, error);
    else
        g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

void
mm_shared_cinterion_disable_location_gathering (MMIfaceModemLocation  *self,
                                                MMModemLocationSource  source,
                                                GAsyncReadyCallback    callback,
                                                gpointer               user_data)
{
    DisableLocationGatheringContext *ctx;
    MMModemLocationSource            enabled_sources;
    Private                         *priv;
    GTask                           *task;

    task = g_task_new (self, NULL, callback, user_data);

    priv = get_private (MM_SHARED_CINTERION (self));
    g_assert (priv->iface_modem_location_parent);

    /* Only consider request if it applies to one of the sources we are
     * supporting, otherwise run parent disable */
    if (!(priv->supported_sources & source)) {
        /* If disabling implemented by the parent, run it. */
        if (priv->iface_modem_location_parent->disable_location_gathering &&
            priv->iface_modem_location_parent->disable_location_gathering_finish) {
            priv->iface_modem_location_parent->disable_location_gathering (self,
                                                                           source,
                                                                           (GAsyncReadyCallback)parent_disable_location_gathering_ready,
                                                                           task);
            return;
        }
        /* Otherwise, we're done */
        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        return;
    }

    /* We only expect GPS sources here */
    g_assert (source & (MM_MODEM_LOCATION_SOURCE_GPS_NMEA |
                        MM_MODEM_LOCATION_SOURCE_GPS_RAW |
                        MM_MODEM_LOCATION_SOURCE_GPS_UNMANAGED));

    /* Flag as disabled to see how many others we would have left enabled */
    enabled_sources = priv->enabled_sources;
    enabled_sources &= ~source;

    /* If there are still GPS-related sources enabled, do nothing else */
    if (enabled_sources & (MM_MODEM_LOCATION_SOURCE_GPS_NMEA |
                           MM_MODEM_LOCATION_SOURCE_GPS_RAW |
                           MM_MODEM_LOCATION_SOURCE_GPS_UNMANAGED)) {
        priv->enabled_sources &= ~source;
        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        return;
    }

    /* Stop GPS engine if all GPS-related sources are disabled */
    ctx = g_slice_new0 (DisableLocationGatheringContext);
    ctx->source = source;
    ctx->gps_step = DISABLE_LOCATION_GATHERING_GPS_STEP_FIRST;
    g_task_set_task_data (task, ctx, (GDestroyNotify) disable_location_gathering_context_free);
    disable_location_gathering_context_gps_step (task);
}

/*****************************************************************************/
/* Enable location gathering (Location interface) */

/* We will retry the SGPSC command that enables the Engine */
#define MAX_SGPSC_ENGINE_RETRIES 3

/* Cinterion asks for 100ms some time between GPS commands, but we'll give up
 * to 2000ms before setting the Engine configuration as 100ms didn't seem always
 * enough (we would get +CME ERROR: 767 errors reported). */
#define GPS_COMMAND_TIMEOUT_DEFAULT_MS  100
#define GPS_COMMAND_TIMEOUT_ENGINE_MS  2000

typedef enum {
    ENABLE_LOCATION_GATHERING_GPS_STEP_FIRST,
    ENABLE_LOCATION_GATHERING_GPS_STEP_SGPSS,
    ENABLE_LOCATION_GATHERING_GPS_STEP_SGPSC_OUTPUT,
    ENABLE_LOCATION_GATHERING_GPS_STEP_SGPSC_ANTENNA,
    ENABLE_LOCATION_GATHERING_GPS_STEP_SGPSC_ENGINE,
    ENABLE_LOCATION_GATHERING_GPS_STEP_LAST,
} EnableLocationGatheringGpsStep;

typedef struct {
    MMModemLocationSource          source;
    EnableLocationGatheringGpsStep gps_step;
    guint                          sgpsc_engine_retries;
} EnableLocationGatheringContext;

static void
enable_location_gathering_context_free (EnableLocationGatheringContext *ctx)
{
    g_slice_free (EnableLocationGatheringContext, ctx);
}

gboolean
mm_shared_cinterion_enable_location_gathering_finish (MMIfaceModemLocation  *self,
                                                      GAsyncResult          *res,
                                                      GError               **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void enable_location_gathering_context_gps_step (GTask *task);

static gboolean
enable_location_gathering_context_gps_step_schedule_cb (GTask *task)
{
    /* Run the scheduled step */
    enable_location_gathering_context_gps_step (task);
    return G_SOURCE_REMOVE;
}

static void
enable_sgpsc_or_sgpss_ready (MMBaseModem  *self,
                             GAsyncResult *res,
                             GTask        *task)
{
    EnableLocationGatheringContext *ctx;
    GError                         *error = NULL;

    ctx = (EnableLocationGatheringContext *) g_task_get_task_data (task);

    if (!mm_base_modem_at_command_finish (self, res, &error)) {
        /* The GPS setup may sometimes report "+CME ERROR 767" when enabling the
         * Engine; so we'll run some retries of the same command ourselves. */
        if (ctx->gps_step == ENABLE_LOCATION_GATHERING_GPS_STEP_SGPSC_ENGINE) {
            ctx->sgpsc_engine_retries++;
            mm_dbg ("GPS Engine setup failed (%u/%u)", ctx->sgpsc_engine_retries, MAX_SGPSC_ENGINE_RETRIES);
            if (ctx->sgpsc_engine_retries < MAX_SGPSC_ENGINE_RETRIES) {
                g_clear_error (&error);
                goto schedule;
            }
        }
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    /* Go on to next step */
    ctx->gps_step++;

schedule:
    g_timeout_add (ctx->gps_step == ENABLE_LOCATION_GATHERING_GPS_STEP_SGPSC_ENGINE ? GPS_COMMAND_TIMEOUT_ENGINE_MS : GPS_COMMAND_TIMEOUT_DEFAULT_MS,
                   (GSourceFunc) enable_location_gathering_context_gps_step_schedule_cb, task);
}

static void
enable_location_gathering_context_gps_step (GTask *task)
{
    EnableLocationGatheringContext *ctx;
    MMSharedCinterion              *self;
    Private                        *priv;

    self = MM_SHARED_CINTERION (g_task_get_source_object (task));
    priv = get_private (self);
    ctx = (EnableLocationGatheringContext *) g_task_get_task_data (task);

    /* Only one of both supported */
    g_assert ((priv->sgpss_support == FEATURE_SUPPORTED) || (priv->sgpsc_support == FEATURE_SUPPORTED));
    g_assert (!((priv->sgpss_support == FEATURE_SUPPORTED) && (priv->sgpsc_support == FEATURE_SUPPORTED)));

    switch (ctx->gps_step) {
    case ENABLE_LOCATION_GATHERING_GPS_STEP_FIRST:
        ctx->gps_step++;
        /* Fall down to next step */

    case ENABLE_LOCATION_GATHERING_GPS_STEP_SGPSS:
        if (priv->sgpss_support == FEATURE_SUPPORTED) {
            mm_base_modem_at_command (MM_BASE_MODEM (self),
                                      "AT^SGPSS=4",
                                      3, FALSE, (GAsyncReadyCallback) enable_sgpsc_or_sgpss_ready, task);
            return;
        }
        ctx->gps_step++;
        /* Fall down to next step */

    case ENABLE_LOCATION_GATHERING_GPS_STEP_SGPSC_OUTPUT:
        if (priv->sgpsc_support == FEATURE_SUPPORTED) {
            /* NMEA output off */
            mm_base_modem_at_command (MM_BASE_MODEM (self),
                                      "AT^SGPSC=\"NMEA/Output\",\"on\"",
                                      3, FALSE, (GAsyncReadyCallback) enable_sgpsc_or_sgpss_ready, task);
            return;
        }
        ctx->gps_step++;
        /* Fall down to next step */


    case ENABLE_LOCATION_GATHERING_GPS_STEP_SGPSC_ANTENNA:
        if (priv->sgpsc_support == FEATURE_SUPPORTED) {
            /* Antenna off */
            mm_base_modem_at_command (MM_BASE_MODEM (self),
                                      "AT^SGPSC=\"Power/Antenna\",\"on\"",
                                      3, FALSE, (GAsyncReadyCallback) enable_sgpsc_or_sgpss_ready, task);
            return;
        }
        ctx->gps_step++;
        /* Fall down to next step */

    case ENABLE_LOCATION_GATHERING_GPS_STEP_SGPSC_ENGINE:
        if (priv->sgpsc_support == FEATURE_SUPPORTED) {
            /* Engine off */
            mm_base_modem_at_command (MM_BASE_MODEM (self),
                                      "AT^SGPSC=\"Engine\",\"1\"",
                                      3, FALSE, (GAsyncReadyCallback) enable_sgpsc_or_sgpss_ready, task);
            return;
        }
        ctx->gps_step++;
        /* Fall down to next step */

    case ENABLE_LOCATION_GATHERING_GPS_STEP_LAST:
        /* Only use the GPS port in NMEA/RAW setups */
        if (ctx->source & (MM_MODEM_LOCATION_SOURCE_GPS_NMEA |
                           MM_MODEM_LOCATION_SOURCE_GPS_RAW)) {
            MMPortSerialGps *gps_port;
            GError          *error = NULL;

            gps_port = mm_base_modem_peek_port_gps (MM_BASE_MODEM (self));
            if (!gps_port || !mm_port_serial_open (MM_PORT_SERIAL (gps_port), &error)) {
                if (error)
                    g_task_return_error (task, error);
                else
                    g_task_return_new_error (task, MM_CORE_ERROR, MM_CORE_ERROR_FAILED,
                                             "Couldn't open raw GPS serial port");
                g_object_unref (task);
                return;
            }
        }

        /* Success */
        priv->enabled_sources |= ctx->source;
        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        return;
    }
}

static void
parent_enable_location_gathering_ready (MMIfaceModemLocation *self,
                                        GAsyncResult         *res,
                                        GTask                *task)
{
    GError  *error;
    Private *priv;

    priv = get_private (MM_SHARED_CINTERION (self));

    g_assert (priv->iface_modem_location_parent);
    if (!priv->iface_modem_location_parent->enable_location_gathering_finish (self, res, &error))
        g_task_return_error (task, error);
    else
        g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

void
mm_shared_cinterion_enable_location_gathering (MMIfaceModemLocation  *self,
                                               MMModemLocationSource  source,
                                               GAsyncReadyCallback    callback,
                                               gpointer               user_data)
{
    Private                        *priv;
    GTask                          *task;
    EnableLocationGatheringContext *ctx;


    task = g_task_new (self, NULL, callback, user_data);

    priv = get_private (MM_SHARED_CINTERION (self));
    g_assert (priv->iface_modem_location_parent);
    g_assert (priv->iface_modem_location_parent->enable_location_gathering);
    g_assert (priv->iface_modem_location_parent->enable_location_gathering_finish);

    /* Only consider request if it applies to one of the sources we are
     * supporting, otherwise run parent enable */
    if (!(priv->supported_sources & source)) {
        priv->iface_modem_location_parent->enable_location_gathering (self,
                                                                      source,
                                                                      (GAsyncReadyCallback)parent_enable_location_gathering_ready,
                                                                      task);
        return;
    }

    /* We only expect GPS sources here */
    g_assert (source & (MM_MODEM_LOCATION_SOURCE_GPS_NMEA |
                        MM_MODEM_LOCATION_SOURCE_GPS_RAW |
                        MM_MODEM_LOCATION_SOURCE_GPS_UNMANAGED));

    /* If GPS already started, store new flag and we're done */
    if (priv->enabled_sources & (MM_MODEM_LOCATION_SOURCE_GPS_NMEA |
                                 MM_MODEM_LOCATION_SOURCE_GPS_RAW |
                                 MM_MODEM_LOCATION_SOURCE_GPS_UNMANAGED)) {
        priv->enabled_sources |= source;
        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        return;
    }

    ctx = g_slice_new0 (EnableLocationGatheringContext);
    ctx->source   = source;
    ctx->gps_step = ENABLE_LOCATION_GATHERING_GPS_STEP_FIRST;
    g_task_set_task_data (task, ctx, (GDestroyNotify) enable_location_gathering_context_free);

    enable_location_gathering_context_gps_step (task);
}

/*****************************************************************************/

static void
shared_cinterion_init (gpointer g_iface)
{
}

GType
mm_shared_cinterion_get_type (void)
{
    static GType shared_cinterion_type = 0;

    if (!G_UNLIKELY (shared_cinterion_type)) {
        static const GTypeInfo info = {
            sizeof (MMSharedCinterion),  /* class_size */
            shared_cinterion_init,       /* base_init */
            NULL,                  /* base_finalize */
        };

        shared_cinterion_type = g_type_register_static (G_TYPE_INTERFACE, "MMSharedCinterion", &info, 0);
        g_type_interface_add_prerequisite (shared_cinterion_type, MM_TYPE_IFACE_MODEM_LOCATION);
    }

    return shared_cinterion_type;
}
