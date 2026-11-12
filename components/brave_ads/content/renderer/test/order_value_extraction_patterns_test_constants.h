/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_TEST_ORDER_VALUE_EXTRACTION_PATTERNS_TEST_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_TEST_ORDER_VALUE_EXTRACTION_PATTERNS_TEST_CONSTANTS_H_

#include <string_view>

// Mirrors the real `conversions.order.value.regex` patterns delivered on
// the Sponsored Images Component; keep these in sync with the real
// patterns whenever they change, so drift between the two cannot hide a
// real regression from these tests.

namespace brave_ads::test {

inline constexpr std::string_view kEurOrderValueCurrencyRegex =
    R"RE2((?i)(?:(?:(?:EUR|€)[\s\p{Zs}]*)+(?:\d{1,3}(?:,\d{3})+(?:\.\d{2})?|\d{1,3}(?:\.\d{3})+(?:,\d{2})?|\d{1,3}(?:\p{Zs}\d{3})+(?:[.,]\d{2})?|\d+(?:[.,]\d{2})?)(?:[\s\p{Zs}]*(?:EUR|€))*(?:$|[^0-9.,])|(?:^|[^0-9.,])(?:\d{1,3}(?:,\d{3})+(?:\.\d{2})?|\d{1,3}(?:\.\d{3})+(?:,\d{2})?|\d{1,3}(?:\p{Zs}\d{3})+(?:[.,]\d{2})?|\d+(?:[.,]\d{2})?)(?:[\s\p{Zs}]*(?:EUR|€))+(?:$|[^0-9.,])|^[\s\p{Zs}]*(?:\d{1,3}(?:,\d{3})+\.\d{2}|\d{1,3}(?:\.\d{3})+,\d{2}|\d{1,3}(?:\p{Zs}\d{3})+[.,]\d{2}|\d+[.,]\d{2})[\s\p{Zs}]*$))RE2";

inline constexpr std::string_view kUsdOrderValueCurrencyRegex =
    R"RE2((?i)(?:(?:(?:US\$|USD|\$)[\s\p{Zs}]*)+(?:\d{1,3}(?:,\d{3})+(?:\.\d{2})?|\d{1,3}(?:\.\d{3})+(?:,\d{2})?|\d{1,3}(?:\p{Zs}\d{3})+(?:[.,]\d{2})?|\d+(?:[.,]\d{2})?)(?:[\s\p{Zs}]*(?:US\$|USD|\$))*(?:$|[^0-9.,])|(?:^|[^0-9.,])(?:\d{1,3}(?:,\d{3})+(?:\.\d{2})?|\d{1,3}(?:\.\d{3})+(?:,\d{2})?|\d{1,3}(?:\p{Zs}\d{3})+(?:[.,]\d{2})?|\d+(?:[.,]\d{2})?)(?:[\s\p{Zs}]*(?:US\$|USD|\$))+(?:$|[^0-9.,])|^[\s\p{Zs}]*(?:\d{1,3}(?:,\d{3})+\.\d{2}|\d{1,3}(?:\.\d{3})+,\d{2}|\d{1,3}(?:\p{Zs}\d{3})+[.,]\d{2}|\d+[.,]\d{2})[\s\p{Zs}]*$))RE2";

inline constexpr std::string_view kDeOrderValueKeywordRegex =
    R"RE2((?i)^[\s\p{Zs}]*(?:Gesamt(?:(?:preis|betrag|summe|kosten)|(?: zu zahlen))?|Bestellsumme|Bestellgesamtbetrag|Bestellwert|Rechnungsbetrag|Endbetrag|Endpreis|Endsumme|Zu zahlender Betrag|Jetzt zu zahlen|Heute zu zahlen|Heute fällig|Zahlungsbetrag|Zahlung fällig|Zu bezahlen|Zu zahlender Gesamtbetrag|Preis|Ihr Preis|Dein Preis|Bestellübersicht|Zusammenfassung der Bestellung|Summe|Betrag fällig|Fälliger Betrag|Gesamtsumme(?: der Bestellung)?|Warenkorbgesamtbetrag|Gesamt[\s\p{Zs}]*\(inkl\.[\s\p{Zs}]*MwSt\.\)|Sie bezahlen|Gesamtpreis[\s\p{Zs}]*\([\s\p{Zs}]*(?:€|EUR)[\s\p{Zs}]*\)|Gesamt(?:betrag|preis|summe)[\s\p{Zs}]*\([\s\p{Zs}]*inkl\.[\s\p{Zs}]*(?:MwSt\.|USt\.)[\s\p{Zs}]*\)|Gesamtbetrag[\s\p{Zs}]+(?:der|Ihrer|deiner)[\s\p{Zs}]+Bestellung|Bestellbetrag|Zahlbetrag|Zu zahlen|Noch zu zahlen|Zu zahlende Summe|Sie zahlen)[\s\p{Zs}]*:?[\s\p{Zs}]*$)RE2";

inline constexpr std::string_view kEnOrderValueKeywordRegex =
    R"RE2((?i)^(?:[\s\p{Zs}]*)(Due now \(USD\)|(Estimated )?(?:Order Total|Total:?)|TOTAL CHARGED TODAY \*|Final Total Price:|Flight total|grand total:?|Order Total(?: \(USD\))?:?|Price|Total(?:(?: \(USD\))?| Due| for Stay| Price| to be paid:| to pay|:)?|(Your )?(?:Payment Today|total([\s\p{Zs}])price|Total:)|Show Order Summary:|Reservation Deposit Amount Due|Payment Due Now|Your Price|Total Due Now|Amount to pay|Total amount due|You pay today|Order Summary|TOTAL PAYMENT DUE|Payable Amount|(?:Total|Amount)[\s\p{Zs}]+(?:Payable|Due|to[\s\p{Zs}]+Pay)|(?:Total|Order)[\s\p{Zs}]+(?:Amount|Price|Cost|Value)|Final[\s\p{Zs}]+(?:Amount|Total|Price)|Total[\s\p{Zs}]+(?:(?:inc\.?|incl\.?|including)[\s\p{Zs}]*(?:VAT|sales[\s\p{Zs}]+tax|tax(?:es)?)|(?:VAT|sales[\s\p{Zs}]+tax|tax(?:es)?)[\s\p{Zs}]+included)|Total[\s\p{Zs}]*\([\s\p{Zs}]*(?:(?:inc\.?|incl\.?|including)[\s\p{Zs}]*(?:VAT|sales[\s\p{Zs}]+tax|tax(?:es)?)|(?:VAT|sales[\s\p{Zs}]+tax|tax(?:es)?)[\s\p{Zs}]+included)[\s\p{Zs}]*\))(?:[\s\p{Zs}]*)$|(?i:^[\s\p{Zs}]*(?:Total[\s\p{Zs}]*Price|Grand[\s\p{Zs}]*Total|Order[\s\p{Zs}]*Total|Total[\s\p{Zs}]*Due|Amount[\s\p{Zs}]*Due)[\s\p{Zs}]*:[\s\p{Zs}]*(?:(?:(?:[A-Z]{2,3}\p{Sc}|[A-Z]{3}|\p{Sc})[\s\p{Zs}]*)+(?:\d{1,3}(?:[.,\p{Zs}]\d{3})+|\d+)(?:[.,]\d{2})?(?:[\s\p{Zs}]*(?:[A-Z]{2,3}\p{Sc}|[A-Z]{3}|\p{Sc}))*|(?:\d{1,3}(?:[.,\p{Zs}]\d{3})+|\d+)(?:[.,]\d{2})?(?:[\s\p{Zs}]*(?:[A-Z]{2,3}\p{Sc}|[A-Z]{3}|\p{Sc}))+)[\s\p{Zs}]*$)RE2";

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_TEST_ORDER_VALUE_EXTRACTION_PATTERNS_TEST_CONSTANTS_H_
