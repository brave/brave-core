/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_TEST_ORDER_ID_EXTRACTION_PATTERNS_TEST_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_TEST_ORDER_ID_EXTRACTION_PATTERNS_TEST_CONSTANTS_H_

#include <cstdint>

// Copied from the delivered `conversions.order.id` patterns and defaults;
// keep in sync so drift cannot hide a real regression from these tests.

namespace brave_ads::test {

// `id.search.defaultMaxDescendantDepth`.
inline constexpr uint32_t kDefaultOrderIdMaxDescendantLevelsToSearch = 32;

// `id.search.defaultMaxCombinedTextLengthInBytes`.
inline constexpr uint32_t kDefaultOrderIdMaxCombinedTextLengthToSearch = 8192;

// `id.validation.defaultPattern`.
inline constexpr char kDefaultOrderIdValidationPattern[] = "^[A-Za-z0-9]{8}$";

inline constexpr char kDeOrderIdRegex[] =
    R"RE2((?i)^[\s\p{Zs}]*(?:(?:Deine|Ihre|Ihr|Dein|Unsere)[\s\p{Zs}]+)?(?:Bestell|Auftrags|Buchungs|Bestätigungs|Reservierungs)(?:nummer|[\s\p{Zs}-]*nr\.?|[\s\p{Zs}-]*ID)[\s\p{Zs}]*[:#.]?[\s\p{Zs}]*#?[\s\p{Zs}]*([A-Za-z0-9][A-Za-z0-9_/-]{5,39})[\s\p{Zs}]*$)RE2";

inline constexpr char kEnOrderIdRegex[] =
    R"RE2((?i)^[\s\p{Zs}]*(?:(?:Your|The)[\s\p{Zs}]+)?(?:Order|Booking|Reservation|Confirmation)(?:[\s\p{Zs}]+(?:Number|No\.?|ID|Reference|Ref\.?|Code)|[\s\p{Zs}]*#)[\s\p{Zs}]*[:#.]?[\s\p{Zs}]*#?[\s\p{Zs}]*([A-Za-z0-9][A-Za-z0-9_/-]{5,39})[\s\p{Zs}]*$)RE2";

inline constexpr char kFrOrderIdRegex[] =
    R"RE2((?i)^[\s\p{Zs}]*(?:(?:Votre|Le|La)[\s\p{Zs}]+)?(?:(?:Numéro|N°|Nº|No\.?|Référence|Réf\.?)[\s\p{Zs}]+(?:de[\s\p{Zs}]+|d[\x27’])?(?:commande|réservation|confirmation|dossier)|(?:Commande|Réservation|Confirmation)[\s\p{Zs}]+(?:N°|Nº|No\.?|Numéro|Réf\.?))[\s\p{Zs}]*[:#.]?[\s\p{Zs}]*#?[\s\p{Zs}]*([A-Za-z0-9][A-Za-z0-9_/-]{5,39})[\s\p{Zs}]*$)RE2";

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_TEST_ORDER_ID_EXTRACTION_PATTERNS_TEST_CONSTANTS_H_
