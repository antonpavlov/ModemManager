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
 * Copyright (C) 2011 Red Hat, Inc.
 */

#define _GNU_SOURCE
#include <config.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include <ModemManager.h>
#include <mm-errors-types.h>

#if defined WITH_QMI
#include <libqmi-glib.h>
#endif

#if defined WITH_MBIM
#include <libmbim-glib.h>
#endif

#include "mm-log.h"

enum {
    TS_FLAG_NONE = 0,
    TS_FLAG_WALL,
    TS_FLAG_REL
};

static gboolean ts_flags = TS_FLAG_NONE;
static guint32 log_level = MM_LOG_LEVEL_INFO | MM_LOG_LEVEL_WARN | MM_LOG_LEVEL_ERR;
static GTimeVal rel_start = { 0, 0 };
static int logfd = -1;
static gboolean append_log_level_text = TRUE;

typedef struct {
    guint32 num;
    const char *name;
} LogDesc;

static const LogDesc level_descs[] = {
    { MM_LOG_LEVEL_ERR, "ERR" },
    { MM_LOG_LEVEL_WARN | MM_LOG_LEVEL_ERR, "WARN" },
    { MM_LOG_LEVEL_INFO | MM_LOG_LEVEL_WARN | MM_LOG_LEVEL_ERR, "INFO" },
    { MM_LOG_LEVEL_DEBUG | MM_LOG_LEVEL_INFO | MM_LOG_LEVEL_WARN | MM_LOG_LEVEL_ERR, "DEBUG" },
    { 0, NULL }
};

static GString *msgbuf = NULL;
static volatile gsize msgbuf_once = 0;

static int
mm_to_syslog_priority (MMLogLevel level)
{
    switch (level) {
    case MM_LOG_LEVEL_DEBUG:
        return LOG_DEBUG;
    case MM_LOG_LEVEL_WARN:
        return LOG_WARNING;
    case MM_LOG_LEVEL_INFO:
        return LOG_INFO;
    case MM_LOG_LEVEL_ERR:
        return LOG_ERR;
    }
    g_assert_not_reached();
    return 0;
}

static int
glib_to_syslog_priority (GLogLevelFlags level)
{
    switch (level) {
    case G_LOG_LEVEL_ERROR:
        return LOG_CRIT;
    case G_LOG_LEVEL_CRITICAL:
        return LOG_ERR;
    case G_LOG_LEVEL_WARNING:
        return LOG_WARNING;
    case G_LOG_LEVEL_MESSAGE:
        return LOG_NOTICE;
    case G_LOG_LEVEL_DEBUG:
        return LOG_DEBUG;
    default:
        return LOG_INFO;
    }
}

static const char *
log_level_description (MMLogLevel level)
{
    switch (level) {
    case MM_LOG_LEVEL_DEBUG:
        return "<debug>";
    case MM_LOG_LEVEL_WARN:
        return "<warn> ";
    case MM_LOG_LEVEL_INFO:
        return "<info> ";
    case MM_LOG_LEVEL_ERR:
        return "<error>";
    }
    g_assert_not_reached();
    return NULL;
}

void
_mm_log (const char *loc,
         const char *func,
         MMLogLevel level,
         const char *fmt,
         ...)
{
    va_list args;
    GTimeVal tv;
    ssize_t ign;

    if (!(log_level & level))
        return;

    if (g_once_init_enter (&msgbuf_once)) {
        msgbuf = g_string_sized_new (512);
        g_once_init_leave (&msgbuf_once, 1);
    } else
        g_string_truncate (msgbuf, 0);

    if (append_log_level_text)
        g_string_append_printf (msgbuf, "%s ", log_level_description (level));

    if (ts_flags == TS_FLAG_WALL) {
        g_get_current_time (&tv);
        g_string_append_printf (msgbuf, "[%09ld.%06ld] ", tv.tv_sec, tv.tv_usec);
    } else if (ts_flags == TS_FLAG_REL) {
        glong secs;
        glong usecs;

        g_get_current_time (&tv);
        secs = tv.tv_sec - rel_start.tv_sec;
        usecs = tv.tv_usec - rel_start.tv_usec;
        if (usecs < 0) {
            secs--;
            usecs += 1000000;
        }

        g_string_append_printf (msgbuf, "[%06ld.%06ld] ", secs, usecs);
    }

#if defined MM_LOG_FUNC_LOC
    g_string_append_printf (msgbuf, "[%s] %s(): ", loc, func);
#endif

    va_start (args, fmt);
    g_string_append_vprintf (msgbuf, fmt, args);
    va_end (args);

    g_string_append_c (msgbuf, '\n');

    if (logfd < 0)
        syslog (mm_to_syslog_priority (level), "%s", msgbuf->str);
    else {
        ign = write (logfd, msgbuf->str, msgbuf->len);
        if (ign) {} /* whatever; really shut up about unused result */

        fsync (logfd);  /* Make sure output is dumped to disk immediately */
    }
}

static void
log_handler (const gchar *log_domain,
             GLogLevelFlags level,
             const gchar *message,
             gpointer ignored)
{
    ssize_t ign;

    if (logfd < 0)
        syslog (glib_to_syslog_priority (level), "%s", message);
    else {
        ign = write (logfd, message, strlen (message));
        if (ign) {} /* whatever; really shut up about unused result */
    }
}

gboolean
mm_log_set_level (const char *level, GError **error)
{
    gboolean found = FALSE;
    const LogDesc *diter;

    for (diter = &level_descs[0]; diter->name; diter++) {
        if (!strcasecmp (diter->name, level)) {
            log_level = diter->num;
            found = TRUE;
            break;
        }
    }

    if (!found)
        g_set_error (error, MM_CORE_ERROR, MM_CORE_ERROR_INVALID_ARGS,
                     "Unknown log level '%s'", level);

#if defined WITH_QMI
    qmi_utils_set_traces_enabled (log_level & MM_LOG_LEVEL_DEBUG ? TRUE : FALSE);
#endif

#if defined WITH_MBIM
    mbim_utils_set_traces_enabled (log_level & MM_LOG_LEVEL_DEBUG ? TRUE : FALSE);
#endif

    return found;
}

gboolean
mm_log_setup (const char *level,
              const char *log_file,
              gboolean show_timestamps,
              gboolean rel_timestamps,
              GError **error)
{
    /* levels */
    if (level && strlen (level) && !mm_log_set_level (level, error))
        return FALSE;

    if (show_timestamps)
        ts_flags = TS_FLAG_WALL;
    else if (rel_timestamps)
        ts_flags = TS_FLAG_REL;

    /* Grab start time for relative timestamps */
    g_get_current_time (&rel_start);

    if (log_file == NULL)
        openlog (G_LOG_DOMAIN, LOG_CONS | LOG_PID | LOG_PERROR, LOG_DAEMON);
    else {
        logfd = open (log_file,
                      O_CREAT | O_APPEND | O_WRONLY,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        if (logfd < 0) {
            g_set_error (error, MM_CORE_ERROR, MM_CORE_ERROR_FAILED,
                         "Couldn't open log file: (%d) %s",
                         errno, strerror (errno));
            return FALSE;
        }
    }

    g_log_set_handler (G_LOG_DOMAIN,
                       G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
                       log_handler,
                       NULL);

#if defined WITH_QMI
    g_log_set_handler ("Qmi",
                       G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
                       log_handler,
                       NULL);
#endif

#if defined WITH_MBIM
    g_log_set_handler ("Mbim",
                       G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
                       log_handler,
                       NULL);
#endif

    return TRUE;
}

void
mm_log_shutdown (void)
{
    if (logfd < 0)
        closelog ();
    else
        close (logfd);
}
