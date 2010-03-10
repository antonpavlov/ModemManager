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
 * Copyright (C) 2008 - 2009 Novell, Inc.
 * Copyright (C) 2009 - 2010 Red Hat, Inc.
 */

#include <glib.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "mm-errors.h"
#include "mm-modem-helpers.h"

static void
save_scan_value (GHashTable *hash, const char *key, GMatchInfo *info, guint32 num)
{
    char *quoted;
    size_t len;

    g_return_if_fail (info != NULL);

    quoted = g_match_info_fetch (info, num);
    if (!quoted)
        return;

    len = strlen (quoted);

    /* Unquote the item if needed */
    if ((len >= 2) && (quoted[0] == '"') && (quoted[len - 1] == '"')) {
        quoted[0] = ' ';
        quoted[len - 1] = ' ';
        quoted = g_strstrip (quoted);
    }

    if (!strlen (quoted)) {
        g_free (quoted);
        return;
    }

    g_hash_table_insert (hash, g_strdup (key), quoted);
}

/* If the response was successfully parsed (even if no valid entries were
 * found) the pointer array will be returned.
 */
GPtrArray *
mm_gsm_parse_scan_response (const char *reply, GError **error)
{
    /* Got valid reply */
    GPtrArray *results = NULL;
    GRegex *r;
    GMatchInfo *match_info;
    GError *err = NULL;
    gboolean umts_format = TRUE;

    g_return_val_if_fail (reply != NULL, NULL);
    if (error)
        g_return_val_if_fail (*error == NULL, NULL);

    if (!strstr (reply, "+COPS: ")) {
        g_set_error_literal (error,
                             MM_MODEM_ERROR, MM_MODEM_ERROR_GENERAL,
                             "Could not parse scan results.");
        return NULL;
    }

    reply = strstr (reply, "+COPS: ") + 7;

    /* Cell access technology (GSM, UTRAN, etc) got added later and not all
     * modems implement it.  Some modesm have quirks that make it hard to
     * use one regular experession for matching both pre-UMTS and UMTS
     * responses.  So try UMTS-format first and fall back to pre-UMTS if
     * we get no UMTS-formst matches.
     */

    /* Quirk: Sony-Ericsson TM-506 sometimes includes a stray ')' like so,
     *        which is what makes it hard to match both pre-UMTS and UMTS in
     *        the same regex:
     *
     *       +COPS: (2,"","T-Mobile","31026",0),(1,"AT&T","AT&T","310410"),0)
     */

    r = g_regex_new ("\\((\\d),([^,\\)]*),([^,\\)]*),([^,\\)]*)[\\)]?,(\\d)\\)", G_REGEX_UNGREEDY, 0, NULL);
    if (err) {
        g_error ("Invalid regular expression: %s", err->message);
        g_error_free (err);
        g_set_error_literal (error,
                             MM_MODEM_ERROR, MM_MODEM_ERROR_GENERAL,
                             "Could not parse scan results.");
        return NULL;
    }

    /* If we didn't get any hits, try the pre-UMTS format match */
    if (!g_regex_match (r, reply, 0, &match_info)) {
        g_regex_unref (r);
        if (match_info) {
            g_match_info_free (match_info);
            match_info = NULL;
        }

        /* Pre-UMTS format doesn't include the cell access technology after
         * the numeric operator element.
         *
         * Ex: Motorola C-series (BUSlink SCWi275u) like so:
         *
         *       +COPS: (2,"T-Mobile","","310260"),(0,"Cingular Wireless","","310410")
         */

        /* Quirk: Some Nokia phones (N80) don't send the quotes for empty values:
         *
         *       +COPS: (2,"T - Mobile",,"31026"),(1,"Einstein PCS",,"31064"),(1,"Cingular",,"31041"),,(0,1,3),(0,2)
         */

        r = g_regex_new ("\\((\\d),([^,\\)]*),([^,\\)]*),([^\\)]*)\\)", G_REGEX_UNGREEDY, 0, NULL);
        if (err) {
            g_error ("Invalid regular expression: %s", err->message);
            g_error_free (err);
            g_set_error_literal (error,
                                 MM_MODEM_ERROR, MM_MODEM_ERROR_GENERAL,
                                 "Could not parse scan results.");
            return NULL;
        }

        g_regex_match (r, reply, 0, &match_info);
        umts_format = FALSE;
    }

    /* Parse the results */
    results = g_ptr_array_new ();
    while (g_match_info_matches (match_info)) {
        GHashTable *hash;
        char *access_tech = NULL;
        const char *tmp;
        gboolean valid = FALSE;

        hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

        save_scan_value (hash, MM_SCAN_TAG_STATUS, match_info, 1);
        save_scan_value (hash, MM_SCAN_TAG_OPER_LONG, match_info, 2);
        save_scan_value (hash, MM_SCAN_TAG_OPER_SHORT, match_info, 3);
        save_scan_value (hash, MM_SCAN_TAG_OPER_NUM, match_info, 4);

        /* Only try for access technology with UMTS-format matches */
        if (umts_format)
            access_tech = g_match_info_fetch (match_info, 5);
        if (access_tech && (strlen (access_tech) == 1)) {
            /* Recognized access technologies are between '0' and '6' inclusive... */
            if ((access_tech[0] >= '0') && (access_tech[0] <= '6'))
                g_hash_table_insert (hash, g_strdup (MM_SCAN_TAG_ACCESS_TECH), access_tech);
        } else
            g_free (access_tech);

        /* If the operator number isn't valid (ie, at least 5 digits),
         * ignore the scan result; it's probably the parameter stuff at the
         * end of the +COPS response.  The regex will sometimes catch this
         * but there's no good way to ignore it.
         */
        tmp = g_hash_table_lookup (hash, MM_SCAN_TAG_OPER_NUM);
        if (tmp && (strlen (tmp) >= 5)) {
            valid = TRUE;
            while (*tmp) {
                if (!isdigit (*tmp) && (*tmp != '-')) {
                    valid = FALSE;
                    break;
                }
                tmp++;
            }

            if (valid)
                g_ptr_array_add (results, hash);
        }

        if (!valid)
            g_hash_table_destroy (hash);

        g_match_info_next (match_info, NULL);
    }

    g_match_info_free (match_info);
    g_regex_unref (r);

    return results;
}

void
mm_gsm_destroy_scan_data (gpointer data)
{
    GPtrArray *results = (GPtrArray *) data;

    g_ptr_array_foreach (results, (GFunc) g_hash_table_destroy, NULL);
    g_ptr_array_free (results, TRUE);
}

/*************************************************************************/

/* +CREG: <stat>                       (GSM 07.07 CREG=1 unsolicited) */
#define CREG1 "\\+(CREG|CGREG):\\s*(\\d{1})"

/* +CREG: <n>,<stat>                   (GSM 07.07 CREG=1 solicited) */
#define CREG2 "\\+(CREG|CGREG):\\s*(\\d{1}),\\s*(\\d{1})"

/* +CREG: <stat>,<lac>,<ci>           (GSM 07.07 CREG=2 unsolicited) */
#define CREG3 "\\+(CREG|CGREG):\\s*(\\d{1}),\\s*([^,\\s]*)\\s*,\\s*([^,\\s]*)"

/* +CREG: <n>,<stat>,<lac>,<ci>       (GSM 07.07 solicited and some CREG=2 unsolicited) */
#define CREG4 "\\+(CREG|CGREG):\\s*(\\d{1}),\\s*(\\d{1})\\s*,\\s*([^,\\s]*)\\s*,\\s*([^,\\s]*)"

/* +CREG: <stat>,<lac>,<ci>,<AcT>     (ETSI 27.007 CREG=2 unsolicited) */
#define CREG5 "\\+(CREG|CGREG):\\s*(\\d{1})\\s*,\\s*([^,\\s]*)\\s*,\\s*([^,\\s]*)\\s*,\\s*(\\d{1,2})"

/* +CREG: <n>,<stat>,<lac>,<ci>,<AcT> (ETSI 27.007 solicited and some CREG=2 unsolicited) */
#define CREG6 "\\+(CREG|CGREG):\\s*(\\d{1}),\\s*(\\d{1})\\s*,\\s*([^,\\s]*)\\s*,\\s*([^,\\s]*)\\s*,\\s*(\\d{1,2})"

GPtrArray *
mm_gsm_creg_regex_get (gboolean solicited)
{
    GPtrArray *array = g_ptr_array_sized_new (6);
    GRegex *regex;

    /* #1 */
    if (solicited)
        regex = g_regex_new (CREG1 "$", G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, NULL);
    else
        regex = g_regex_new ("\\r\\n" CREG1 "\\r\\n", G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, NULL);
    g_assert (regex);
    g_ptr_array_add (array, regex);

    /* #2 */
    if (solicited)
        regex = g_regex_new (CREG2 "$", G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, NULL);
    else
        regex = g_regex_new ("\\r\\n" CREG2 "\\r\\n", G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, NULL);
    g_assert (regex);
    g_ptr_array_add (array, regex);

    /* #3 */
    if (solicited)
        regex = g_regex_new (CREG3 "$", G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, NULL);
    else
        regex = g_regex_new ("\\r\\n" CREG3 "\\r\\n", G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, NULL);
    g_assert (regex);
    g_ptr_array_add (array, regex);

    /* #4 */
    if (solicited)
        regex = g_regex_new (CREG4 "$", G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, NULL);
    else
        regex = g_regex_new ("\\r\\n" CREG4 "\\r\\n", G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, NULL);
    g_assert (regex);
    g_ptr_array_add (array, regex);

    /* #5 */
    if (solicited)
        regex = g_regex_new (CREG5 "$", G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, NULL);
    else
        regex = g_regex_new ("\\r\\n" CREG5 "\\r\\n", G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, NULL);
    g_assert (regex);
    g_ptr_array_add (array, regex);

    /* #6 */
    if (solicited)
        regex = g_regex_new (CREG6 "$", G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, NULL);
    else
        regex = g_regex_new ("\\r\\n" CREG6 "\\r\\n", G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, NULL);
    g_assert (regex);
    g_ptr_array_add (array, regex);

    return array;
}

void
mm_gsm_creg_regex_destroy (GPtrArray *array)
{
    g_ptr_array_foreach (array, (GFunc) g_regex_unref, NULL);
    g_ptr_array_free (array, TRUE);
}

/*************************************************************************/

static gulong
parse_uint (char *str, int base, glong nmin, glong nmax, gboolean *valid)
{
    gulong ret = 0;
    char *endquote;

    *valid = FALSE;
    if (!str)
        return 0;

    /* Strip quotes */
    if (str[0] == '"')
        str++;
    endquote = strchr (str, '"');
    if (endquote)
        *endquote = '\0';

    if (strlen (str)) {
        ret = strtol (str, NULL, base);
        if ((nmin == nmax) || (ret >= nmin && ret <= nmax))
            *valid = TRUE;
    }
    return *valid ? (guint) ret : 0;
}

gboolean
mm_gsm_parse_creg_response (GMatchInfo *info,
                            guint32 *out_reg_state,
                            gulong *out_lac,
                            gulong *out_ci,
                            gint *out_act,
                            gboolean *out_cgreg,
                            GError **error)
{
    gboolean success = FALSE, foo;
    gint n_matches, act = -1;
    gulong stat = 0, lac = 0, ci = 0;
    guint istat = 0, ilac = 0, ici = 0, iact = 0;
    char *str;

    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (out_reg_state != NULL, FALSE);
    g_return_val_if_fail (out_lac != NULL, FALSE);
    g_return_val_if_fail (out_ci != NULL, FALSE);
    g_return_val_if_fail (out_act != NULL, FALSE);
    g_return_val_if_fail (out_cgreg != NULL, FALSE);

    str = g_match_info_fetch (info, 1);
    if (str && strstr (str, "CGREG"))
        *out_cgreg = TRUE;

    /* Normally the number of matches could be used to determine what each
     * item is, but we have overlap in one case.
     */
    n_matches = g_match_info_get_match_count (info);
    if (n_matches == 3) {
        /* CREG=1: +CREG: <stat> */
        istat = 2;
    } else if (n_matches == 4) {
        /* Solicited response: +CREG: <n>,<stat> */
        istat = 3;
    } else if (n_matches == 5) {
        /* CREG=2 (GSM 07.07): +CREG: <stat>,<lac>,<ci> */
        istat = 2;
        ilac = 3;
        ici = 4;
    } else if (n_matches == 6) {
        /* CREG=2 (ETSI 27.007): +CREG: <stat>,<lac>,<ci>,<AcT>
         * CREG=2 (non-standard): +CREG: <n>,<stat>,<lac>,<ci>
         */

        /* To distinguish, check length of the third match item.  If it's
         * more than one digit or has quotes in it then it's a LAC and we
         * got the first format.
         */
        str = g_match_info_fetch (info, 3);
        if (str && (strchr (str, '"') || strlen (str) > 1)) {
            g_free (str);
            istat = 2;
            ilac = 3;
            ici = 4;
            iact = 5;
        } else {
            istat = 3;
            ilac = 4;
            ici = 5;
        }
    } else if (n_matches == 7) {
        /* CREG=2 (non-standard): +CREG: <n>,<stat>,<lac>,<ci>,<AcT> */
        istat = 3;
        ilac = 4;
        ici = 5;
        iact = 6;
    }

    /* Status */
    str = g_match_info_fetch (info, istat);
    stat = parse_uint (str, 10, 0, 5, &success);
    g_free (str);
    if (!success) {
        g_set_error_literal (error,
                             MM_MODEM_ERROR, MM_MODEM_ERROR_GENERAL,
                             "Could not parse the registration status response");
        return FALSE;
    }

    /* Location Area Code */
    if (ilac) {
        /* FIXME: some phones apparently swap the LAC bytes (LG, SonyEricsson,
         * Sagem).  Need to handle that.
         */
        str = g_match_info_fetch (info, ilac);
        lac = parse_uint (str, 16, 1, 0xFFFF, &foo);
        g_free (str);
    }

    /* Cell ID */
    if (ici) {
        str = g_match_info_fetch (info, ici);
        ci = parse_uint (str, 16, 1, 0x0FFFFFFE, &foo);
        g_free (str);
    }

    /* Access Technology */
    if (iact) {
        str = g_match_info_fetch (info, iact);
        act = (gint) parse_uint (str, 10, 0, 7, &foo);
        g_free (str);
        if (!foo)
            act = -1;
    }

    *out_reg_state = (guint32) stat;
    if (stat != 4) {
        /* Don't fill in lac/ci/act if the device's state is unknown */
        *out_lac = lac;
        *out_ci = ci;
        *out_act = act;
    }
    return TRUE;
}

