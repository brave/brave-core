/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/test/order_extraction_patterns_resource_test_util.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/file_path_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_json_reader.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_patterns_resource_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_selector_info.h"

namespace brave_ads::test {

namespace {

constexpr char kConversionConfigJsonFilename[] = "conversion-config.json";

constexpr char kSponsoredContentComponentId[] =
    "gccbbckogglekeggclmmekihdgdpdgoe";

}  // namespace

OrderExtractionPatternsInfo ReadConversionConfigFromResource() {
  const base::DictValue dict = base::test::ParseJsonDictFromFile(
      ResourceComponentsDataPath()
          .AppendASCII(kSponsoredContentComponentId)
          .AppendASCII(kConversionConfigJsonFilename));
  std::optional<OrderExtractionPatternsResourceInfo> resource =
      json::reader::MaybeReadOrderExtractionPatternsResource(dict);
  CHECK(resource);
  return std::move(resource->patterns);
}

OrderExtractionPatternsInfo BuildExpectedConversionConfig() {
  OrderExtractionPatternsInfo expected_conversion_config;

  expected_conversion_config.id.regexes["de"] =
      R"RE2((?i)^[\s\p{Zs}]*(?:(?:Deine|Ihre|Ihr|Dein|Unsere)[\s\p{Zs}]+)?(?:Bestell|Auftrags|Buchungs|Bestätigungs|Reservierungs)(?:nummer|[\s\p{Zs}-]*nr\.?|[\s\p{Zs}-]*ID)[\s\p{Zs}]*[:#.]?[\s\p{Zs}]*#?[\s\p{Zs}]*([A-Za-z0-9][A-Za-z0-9_/-]{5,39})[\s\p{Zs}]*$)RE2";
  expected_conversion_config.id.regexes["en"] =
      R"RE2((?i)^[\s\p{Zs}]*(?:(?:Your|The)[\s\p{Zs}]+)?(?:Order|Booking|Reservation|Confirmation)(?:[\s\p{Zs}]+(?:Number|No\.?|ID|Reference|Ref\.?|Code)|[\s\p{Zs}]*#)[\s\p{Zs}]*[:#.]?[\s\p{Zs}]*#?[\s\p{Zs}]*([A-Za-z0-9][A-Za-z0-9_/-]{5,39})[\s\p{Zs}]*$)RE2";
  expected_conversion_config.id.regexes["fr"] =
      R"RE2((?i)^[\s\p{Zs}]*(?:(?:Votre|Le|La)[\s\p{Zs}]+)?(?:(?:Numéro|N°|Nº|No\.?|Référence|Réf\.?)[\s\p{Zs}]+(?:de[\s\p{Zs}]+|d[\x27’])?(?:commande|réservation|confirmation|dossier)|(?:Commande|Réservation|Confirmation)[\s\p{Zs}]+(?:N°|Nº|No\.?|Numéro|Réf\.?))[\s\p{Zs}]*[:#.]?[\s\p{Zs}]*#?[\s\p{Zs}]*([A-Za-z0-9][A-Za-z0-9_/-]{5,39})[\s\p{Zs}]*$)RE2";
  expected_conversion_config.id.default_max_combined_text_length_to_search =
      8192;
  expected_conversion_config.id.default_number_of_descendant_levels_to_search =
      32;
  expected_conversion_config.id.default_validation_pattern =
      R"RE2(^[A-Za-z0-9]{8}$)RE2";

  expected_conversion_config.id
      .validation_pattern_overrides[kWaldoExampleDomain] =
      R"RE2(^WLD[0-9]{2}-[0-9]{12}$)RE2";
  expected_conversion_config.id
      .validation_pattern_overrides[kXyzzyExampleDomain] =
      R"RE2(^[0-9]{10}$)RE2";
  expected_conversion_config.id
      .validation_pattern_overrides[kGraultExampleDomain] =
      R"RE2(^[A-Za-z0-9]{9}$)RE2";
  expected_conversion_config.id
      .validation_pattern_overrides[kFooExampleDomain] = R"RE2(^[0-9]{9}$)RE2";
  expected_conversion_config.id
      .validation_pattern_overrides[kQuxExampleDomain] =
      R"RE2(^[A-Za-z0-9]{9}$)RE2";
  expected_conversion_config.id
      .number_of_descendant_levels_to_search_overrides[kFooExampleDomain] = 33;
  expected_conversion_config.id
      .max_combined_text_length_to_search_overrides[kFooExampleDomain] = 16384;

  OrderExtractionSelectorInfo bar_checkout_id_selector;
  bar_checkout_id_selector.selector = "[data-shopping-cart-id]";
  bar_checkout_id_selector.attribute_name = "data-shopping-cart-id";
  bar_checkout_id_selector.validation_pattern = "^[A-Fa-f0-9]{24}$";
  expected_conversion_config.id
      .selectors[kBarExampleDomain][OrderExtractionPageType::kCheckout] = {
      bar_checkout_id_selector};

  OrderExtractionSelectorInfo bar_thank_you_id_selector;
  bar_thank_you_id_selector.selector = "[data-order-id]";
  bar_thank_you_id_selector.attribute_name = "data-order-id";
  bar_thank_you_id_selector.validation_pattern = "^[A-Fa-f0-9]{24}$";
  expected_conversion_config.id
      .selectors[kBarExampleDomain][OrderExtractionPageType::kThankYou] = {
      bar_thank_you_id_selector};

  OrderExtractionSelectorInfo portal_baz_id_selector;
  portal_baz_id_selector.selector = "script:not([src])";
  portal_baz_id_selector.pattern =
      R"RE2((?i)xvpn:sub:([0-9a-f]{8}(?:-[0-9a-f]{4}){3}-[0-9a-f]{12}))RE2";
  portal_baz_id_selector.validation_pattern =
      R"RE2(^[A-Fa-f0-9]{8}(?:-[A-Fa-f0-9]{4}){3}-[A-Fa-f0-9]{12}$)RE2";
  expected_conversion_config.id
      .selectors[kGarplyBazExampleDomain][OrderExtractionPageType::kThankYou] =
      {portal_baz_id_selector};

  OrderExtractionSelectorInfo quux_id_selector;
  quux_id_selector.selector = R"([data-testid="order-id"] [role="button"] div)";
  quux_id_selector.validation_pattern = "^[0-9]{10}$";
  expected_conversion_config.id
      .selectors[kQuuxExampleDomain][OrderExtractionPageType::kThankYou] = {
      quux_id_selector};

  OrderExtractionSelectorInfo corge_id_selector;
  corge_id_selector.selector = R"([data-testid="orderNumber"])";
  corge_id_selector.validation_pattern = "^[0-9]{14}$";
  expected_conversion_config.id
      .selectors[kCorgeExampleDomain][OrderExtractionPageType::kThankYou] = {
      corge_id_selector};

  expected_conversion_config.value.currency_regexes["EUR"] =
      R"RE2((?i)(?:(?:(?:EUR|€)[\s\p{Zs}]*)+(?:\d{1,3}(?:,\d{3})+(?:\.\d{2})?|\d{1,3}(?:\.\d{3})+(?:,\d{2})?|\d{1,3}(?:\p{Zs}\d{3})+(?:[.,]\d{2})?|\d+(?:[.,]\d{2})?)(?:[\s\p{Zs}]*(?:EUR|€))*(?:$|[^0-9.,])|(?:^|[^0-9.,])(?:\d{1,3}(?:,\d{3})+(?:\.\d{2})?|\d{1,3}(?:\.\d{3})+(?:,\d{2})?|\d{1,3}(?:\p{Zs}\d{3})+(?:[.,]\d{2})?|\d+(?:[.,]\d{2})?)(?:[\s\p{Zs}]*(?:EUR|€))+(?:$|[^0-9.,])|^[\s\p{Zs}]*(?:\d{1,3}(?:,\d{3})+\.\d{2}|\d{1,3}(?:\.\d{3})+,\d{2}|\d{1,3}(?:\p{Zs}\d{3})+[.,]\d{2}|\d+[.,]\d{2})[\s\p{Zs}]*$))RE2";
  expected_conversion_config.value.currency_regexes["USD"] =
      R"RE2((?i)(?:(?:(?:US\$|USD|\$)[\s\p{Zs}]*)+(?:\d{1,3}(?:,\d{3})+(?:\.\d{2})?|\d{1,3}(?:\.\d{3})+(?:,\d{2})?|\d{1,3}(?:\p{Zs}\d{3})+(?:[.,]\d{2})?|\d+(?:[.,]\d{2})?)(?:[\s\p{Zs}]*(?:US\$|USD|\$))*(?:$|[^0-9.,])|(?:^|[^0-9.,])(?:\d{1,3}(?:,\d{3})+(?:\.\d{2})?|\d{1,3}(?:\.\d{3})+(?:,\d{2})?|\d{1,3}(?:\p{Zs}\d{3})+(?:[.,]\d{2})?|\d+(?:[.,]\d{2})?)(?:[\s\p{Zs}]*(?:US\$|USD|\$))+(?:$|[^0-9.,])|^[\s\p{Zs}]*(?:\d{1,3}(?:,\d{3})+\.\d{2}|\d{1,3}(?:\.\d{3})+,\d{2}|\d{1,3}(?:\p{Zs}\d{3})+[.,]\d{2}|\d+[.,]\d{2})[\s\p{Zs}]*$))RE2";
  expected_conversion_config.value.currency_regexes["GBP"] =
      R"RE2((?i)(?:(?:(?:GBP|£)[\s\p{Zs}]*)+(?:\d{1,3}(?:,\d{3})+(?:\.\d{2})?|\d{1,3}(?:\.\d{3})+(?:,\d{2})?|\d{1,3}(?:\p{Zs}\d{3})+(?:[.,]\d{2})?|\d+(?:[.,]\d{2})?)(?:[\s\p{Zs}]*(?:GBP|£))*(?:$|[^0-9.,])|(?:^|[^0-9.,])(?:\d{1,3}(?:,\d{3})+(?:\.\d{2})?|\d{1,3}(?:\.\d{3})+(?:,\d{2})?|\d{1,3}(?:\p{Zs}\d{3})+(?:[.,]\d{2})?|\d+(?:[.,]\d{2})?)(?:[\s\p{Zs}]*(?:GBP|£))+(?:$|[^0-9.,])|^[\s\p{Zs}]*(?:\d{1,3}(?:,\d{3})+\.\d{2}|\d{1,3}(?:\.\d{3})+,\d{2}|\d{1,3}(?:\p{Zs}\d{3})+[.,]\d{2}|\d+[.,]\d{2})[\s\p{Zs}]*$))RE2";
  expected_conversion_config.value.keyword_regexes["de"] =
      R"RE2((?i)^[\s\p{Zs}]*(?:Gesamt(?:(?:preis|betrag|summe|kosten)|(?: zu zahlen))?|Bestellsumme|Bestellgesamtbetrag|Bestellwert|Rechnungsbetrag|Endbetrag|Endpreis|Endsumme|Zu zahlender Betrag|Jetzt zu zahlen|Heute zu zahlen|Heute fällig|Zahlungsbetrag|Zahlung fällig|Zu bezahlen|Zu zahlender Gesamtbetrag|Preis|Ihr Preis|Dein Preis|Bestellübersicht|Zusammenfassung der Bestellung|Summe|Betrag fällig|Fälliger Betrag|Gesamtsumme(?: der Bestellung)?|Warenkorbgesamtbetrag|Gesamt[\s\p{Zs}]*\(inkl\.[\s\p{Zs}]*MwSt\.\)|Sie bezahlen|Gesamtpreis[\s\p{Zs}]*\([\s\p{Zs}]*(?:€|EUR)[\s\p{Zs}]*\)|Gesamt(?:betrag|preis|summe)[\s\p{Zs}]*\([\s\p{Zs}]*inkl\.[\s\p{Zs}]*(?:MwSt\.|USt\.)[\s\p{Zs}]*\)|Gesamtbetrag[\s\p{Zs}]+(?:der|Ihrer|deiner)[\s\p{Zs}]+Bestellung|Bestellbetrag|Zahlbetrag|Zu zahlen|Noch zu zahlen|Zu zahlende Summe|Sie zahlen)[\s\p{Zs}]*:?[\s\p{Zs}]*$)RE2";
  expected_conversion_config.value.keyword_regexes["en"] =
      R"RE2((?i)^(?:[\s\p{Zs}]*)(Due now \(USD\)|(Estimated )?(?:Order Total|Total:?)|TOTAL CHARGED TODAY \*|Final Total Price:|Flight total|grand total:?|Order Total(?: \(USD\))?:?|Price|Total(?:(?: \(USD\))?| Due| for Stay| Price| to be paid:| to pay|:)?|(Your )?(?:Payment Today|total([\s\p{Zs}])price|Total:)|Show Order Summary:|Reservation Deposit Amount Due|Payment Due Now|Your Price|Total Due Now|Amount to pay|Total amount due|You pay today|Order Summary|TOTAL PAYMENT DUE|Payable Amount|(?:Total|Amount)[\s\p{Zs}]+(?:Payable|Due|to[\s\p{Zs}]+Pay)|(?:Total|Order)[\s\p{Zs}]+(?:Amount|Price|Cost|Value)|Final[\s\p{Zs}]+(?:Amount|Total|Price)|Total[\s\p{Zs}]+(?:(?:inc\.?|incl\.?|including)[\s\p{Zs}]*(?:VAT|sales[\s\p{Zs}]+tax|tax(?:es)?)|(?:VAT|sales[\s\p{Zs}]+tax|tax(?:es)?)[\s\p{Zs}]+included)|Total[\s\p{Zs}]*\([\s\p{Zs}]*(?:(?:inc\.?|incl\.?|including)[\s\p{Zs}]*(?:VAT|sales[\s\p{Zs}]+tax|tax(?:es)?)|(?:VAT|sales[\s\p{Zs}]+tax|tax(?:es)?)[\s\p{Zs}]+included)[\s\p{Zs}]*\))(?:[\s\p{Zs}]*)$|(?i:^[\s\p{Zs}]*(?:Total[\s\p{Zs}]*Price|Grand[\s\p{Zs}]*Total|Order[\s\p{Zs}]*Total|Total[\s\p{Zs}]*Due|Amount[\s\p{Zs}]*Due)[\s\p{Zs}]*:[\s\p{Zs}]*(?:(?:(?:[A-Z]{2,3}\p{Sc}|[A-Z]{3}|\p{Sc})[\s\p{Zs}]*)+(?:\d{1,3}(?:[.,\p{Zs}]\d{3})+|\d+)(?:[.,]\d{2})?(?:[\s\p{Zs}]*(?:[A-Z]{2,3}\p{Sc}|[A-Z]{3}|\p{Sc}))*|(?:\d{1,3}(?:[.,\p{Zs}]\d{3})+|\d+)(?:[.,]\d{2})?(?:[\s\p{Zs}]*(?:[A-Z]{2,3}\p{Sc}|[A-Z]{3}|\p{Sc}))+)[\s\p{Zs}]*$))RE2";
  expected_conversion_config.value.keyword_regexes["fr"] =
      R"RE2((?i)^[\s\p{Zs}]*(?:Total(?: de la commande| à payer| TTC)?|Total général|Montant total(?: de la commande| à payer)?|Prix total|Prix final|Prix définitif|Montant à payer|Montant dû|Somme à payer|À payer|À régler|Montant à régler|Paiement dû|Paiement aujourd'hui|À payer aujourd'hui|Payez aujourd'hui|Prix de votre commande|Récapitulatif de commande|Résumé de la commande|Votre total|Votre prix|Total de votre commande|Total à régler|Sous-total à payer|Montant de la commande|Total de votre panier|Total de vos achats|Total de la réservation|Montant de votre commande|Montant de votre réservation|Montant TTC|Montant total TTC|Net à payer|Prix TTC|Prix à payer|Somme due|Montant facturé)[\s\p{Zs}]*:?[\s\p{Zs}]*$)RE2";
  expected_conversion_config.value.default_number_of_ancestor_levels_to_search =
      6;
  expected_conversion_config.value
      .number_of_ancestor_levels_to_search_overrides[kFooExampleDomain] = 7;

  OrderExtractionSelectorInfo waldo_value_selector;
  waldo_value_selector.selector = "script:not([src])";
  waldo_value_selector.pattern =
      R"RE2((?i)"orderTotal"\s*:\s*"?([0-9]+(?:\.[0-9]{2})?)"?)RE2";
  expected_conversion_config.value
      .selectors[kWaldoExampleDomain][OrderExtractionPageType::kThankYou] = {
      waldo_value_selector};

  OrderExtractionSelectorInfo foo_value_selector;
  foo_value_selector.selector = R"([data-test="checkout-total"])";
  expected_conversion_config.value
      .selectors[kFooExampleDomain][OrderExtractionPageType::kCheckout] = {
      foo_value_selector};
  expected_conversion_config.value
      .selectors[kFooExampleDomain][OrderExtractionPageType::kThankYou] = {
      foo_value_selector};

  OrderExtractionSelectorInfo bar_value_selector;
  bar_value_selector.selector = "[data-grand-total]";
  expected_conversion_config.value
      .selectors[kBarExampleDomain][OrderExtractionPageType::kCheckout] = {
      bar_value_selector};

  OrderExtractionSelectorInfo qux_value_selector;
  qux_value_selector.selector =
      R"(aside[data-inspector-id="orderSummary"] strong)";
  expected_conversion_config.value
      .selectors[kQuxExampleDomain][OrderExtractionPageType::kThankYou] = {
      qux_value_selector};

  OrderExtractionSelectorInfo quux_value_selector;
  quux_value_selector.selector =
      R"(button[aria-label="Preisdetails"] > span > span:nth-of-type(2):not([aria-hidden]))";
  expected_conversion_config.value
      .selectors[kQuuxExampleDomain][OrderExtractionPageType::kCheckout] = {
      quux_value_selector};

  return expected_conversion_config;
}

}  // namespace brave_ads::test
