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
 * Copyright (C) 2017 Google, Inc.
 */

#ifndef _MODEMMANAGER_COMPAT_H_
#define _MODEMMANAGER_COMPAT_H_

#if !defined (__MODEM_MANAGER_H_INSIDE__)
#error "Only <ModemManager.h> can be included directly."
#endif

#include <glib.h>

#include <ModemManager-enums.h>

/**
 * SECTION:mm-compat
 * @title: API break replacements
 *
 * These compatibility types and methods are flagged as deprecated and
 * therefore shouldn't be used in newly written code. They are provided to
 * avoid unnecessary API/ABI breaks.
 */

/* The following type exists just so that we can get deprecation warnings */
G_DEPRECATED
typedef int MMModemBandDeprecated;

/**
 * MM_MODEM_BAND_EUTRAN_I:
 *
 * E-UTRAN band 1.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_1 instead.
 */
#define MM_MODEM_BAND_EUTRAN_I ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_1)

/**
 * MM_MODEM_BAND_EUTRAN_II:
 *
 * E-UTRAN band 2.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_2 instead.
 */
#define MM_MODEM_BAND_EUTRAN_II ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_2)

/**
 * MM_MODEM_BAND_EUTRAN_III:
 *
 * E-UTRAN band 3.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_3 instead.
 */
#define MM_MODEM_BAND_EUTRAN_III ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_3)

/**
 * MM_MODEM_BAND_EUTRAN_IV:
 *
 * E-UTRAN band 4.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_4 instead.
 */
#define MM_MODEM_BAND_EUTRAN_IV ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_4)

/**
 * MM_MODEM_BAND_EUTRAN_V:
 *
 * E-UTRAN band 5.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_5 instead.
 */
#define MM_MODEM_BAND_EUTRAN_V ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_5)

/**
 * MM_MODEM_BAND_EUTRAN_VI:
 *
 * E-UTRAN band 6.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_6 instead.
 */
#define MM_MODEM_BAND_EUTRAN_VI ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_6)

/**
 * MM_MODEM_BAND_EUTRAN_VII:
 *
 * E-UTRAN band 7.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_7 instead.
 */
#define MM_MODEM_BAND_EUTRAN_VII ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_7)

/**
 * MM_MODEM_BAND_EUTRAN_VIII:
 *
 * E-UTRAN band 8.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_8 instead.
 */
#define MM_MODEM_BAND_EUTRAN_VIII ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_8)

/**
 * MM_MODEM_BAND_EUTRAN_IX:
 *
 * E-UTRAN band 9.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_9 instead.
 */
#define MM_MODEM_BAND_EUTRAN_IX ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_9)

/**
 * MM_MODEM_BAND_EUTRAN_X:
 *
 * E-UTRAN band 10.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_10 instead.
 */
#define MM_MODEM_BAND_EUTRAN_X ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_10)

/**
 * MM_MODEM_BAND_EUTRAN_XI:
 *
 * E-UTRAN band 11.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_11 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XI ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_11)

/**
 * MM_MODEM_BAND_EUTRAN_XII:
 *
 * E-UTRAN band 12.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_12 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XII ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_12)

/**
 * MM_MODEM_BAND_EUTRAN_XIII:
 *
 * E-UTRAN band 13.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_13 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XIII ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_13)

/**
 * MM_MODEM_BAND_EUTRAN_XIV:
 *
 * E-UTRAN band 14.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_14 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XIV ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_14)

/**
 * MM_MODEM_BAND_EUTRAN_XVII:
 *
 * E-UTRAN band 17.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_17 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XVII ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_17)

/**
 * MM_MODEM_BAND_EUTRAN_XVIII:
 *
 * E-UTRAN band 18.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_18 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XVIII ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_18)

/**
 * MM_MODEM_BAND_EUTRAN_XIX:
 *
 * E-UTRAN band 19.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_19 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XIX ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_19)

/**
 * MM_MODEM_BAND_EUTRAN_XX:
 *
 * E-UTRAN band 20.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_20 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XX ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_20)

/**
 * MM_MODEM_BAND_EUTRAN_XXI:
 *
 * E-UTRAN band 21.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_21 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XXI ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_21)

/**
 * MM_MODEM_BAND_EUTRAN_XXII:
 *
 * E-UTRAN band 22.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_22 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XXII ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_22)

/**
 * MM_MODEM_BAND_EUTRAN_XXIII:
 *
 * E-UTRAN band 23.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_23 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XXIII ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_23)

/**
 * MM_MODEM_BAND_EUTRAN_XXIV:
 *
 * E-UTRAN band 24.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_24 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XXIV ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_24)

/**
 * MM_MODEM_BAND_EUTRAN_XXV:
 *
 * E-UTRAN band 25.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_25 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XXV ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_25)

/**
 * MM_MODEM_BAND_EUTRAN_XXVI:
 *
 * E-UTRAN band 26.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_26 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XXVI ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_26)

/**
 * MM_MODEM_BAND_EUTRAN_XXXIII:
 *
 * E-UTRAN band 33.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_33 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XXXIII ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_33)

/**
 * MM_MODEM_BAND_EUTRAN_XXXIV:
 *
 * E-UTRAN band 34.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_34 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XXXIV ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_34)

/**
 * MM_MODEM_BAND_EUTRAN_XXXV:
 *
 * E-UTRAN band 35.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_35 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XXXV ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_35)

/**
 * MM_MODEM_BAND_EUTRAN_XXXVI:
 *
 * E-UTRAN band 36.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_36 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XXXVI ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_36)

/**
 * MM_MODEM_BAND_EUTRAN_XXXVII:
 *
 * E-UTRAN band 37.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_37 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XXXVII ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_37)

/**
 * MM_MODEM_BAND_EUTRAN_XXXVIII:
 *
 * E-UTRAN band 38.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_38 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XXXVIII ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_38)

/**
 * MM_MODEM_BAND_EUTRAN_XXXIX:
 *
 * E-UTRAN band 39.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_39 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XXXIX ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_39)

/**
 * MM_MODEM_BAND_EUTRAN_XL:
 *
 * E-UTRAN band 40.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_40 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XL ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_40)

/**
 * MM_MODEM_BAND_EUTRAN_XLI:
 *
 * E-UTRAN band 41.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_41 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XLI ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_41)

/**
 * MM_MODEM_BAND_EUTRAN_XLII:
 *
 * E-UTRAN band 42.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_42 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XLII ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_42)

/**
 * MM_MODEM_BAND_EUTRAN_XLIII:
 *
 * E-UTRAN band 43.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_43 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XLIII ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_43)

/**
 * MM_MODEM_BAND_EUTRAN_XLIV:
 *
 * E-UTRAN band 44.
 *
 * Since: 1.0
 * Deprecated: 1.8.0: Use #MM_MODEM_BAND_EUTRAN_44 instead.
 */
#define MM_MODEM_BAND_EUTRAN_XLIV ((MMModemBandDeprecated)MM_MODEM_BAND_EUTRAN_44)

#endif /* _MODEMMANAGER_COMPAT_H_ */
