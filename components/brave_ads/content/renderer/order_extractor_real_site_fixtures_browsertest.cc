/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/test/test_future.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/content/renderer/order_extractor.h"
#include "brave/components/brave_ads/content/renderer/test/order_id_extraction_patterns_test_constants.h"
#include "brave/components/brave_ads/content/renderer/test/order_value_extraction_patterns_test_constants.h"
#include "content/public/test/render_view_test.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*
//
// Fixtures are the full `<body>` DOM hierarchy sourced from real
// checkout/thank-you pages, sanitized (all identifying data replaced with
// synthetic placeholders) while preserving the original element nesting and
// sibling order. Keyword regexes below are prefix-only anchored (`^Label`,
// no trailing `$`), not full-line, since some sites render a label and its
// value combined in a single text node (e.g. booking.com's "Total price:
// US$88.17") rather than split across siblings, and a full-line anchor would
// reject that case.

namespace brave_ads {

namespace {

mojom::OrderSelectorInfoPtr BuildSelector(
    const std::string& selector,
    std::optional<std::string> attribute_name,
    std::optional<std::string> pattern) {
  mojom::OrderSelectorInfoPtr mojom_selector = mojom::OrderSelectorInfo::New();
  mojom_selector->selector = selector;
  mojom_selector->attribute_name = std::move(attribute_name);
  mojom_selector->pattern = std::move(pattern);
  return mojom_selector;
}

mojom::OrderSelectorInfoPtr BuildSelector(const std::string& selector,
                                          std::string attribute_name) {
  return BuildSelector(selector, std::move(attribute_name),
                       /*pattern=*/std::nullopt);
}

mojom::OrderSelectorInfoPtr BuildSelector(const std::string& selector) {
  return BuildSelector(selector, /*attribute_name=*/std::nullopt,
                       /*pattern=*/std::nullopt);
}

}  // namespace

class BraveAdsOrderExtractorRealSiteFixturesRenderViewTest
    : public content::RenderViewTest {
 public:
  void SetUp() override {
    content::RenderViewTest::SetUp();
    registry_ = std::make_unique<service_manager::BinderRegistry>();
    extractor_ =
        std::make_unique<OrderExtractor>(GetMainRenderFrame(), registry_.get());
  }

  void TearDown() override {
    extractor_.reset();
    registry_.reset();
    content::RenderViewTest::TearDown();
  }

 protected:
  mojom::OrderInfoPtr ExtractOrderInfo(
      mojom::OrderPatternsInfoPtr mojom_patterns) {
    base::test::TestFuture<mojom::OrderInfoPtr> test_future;
    extractor_->ExtractOrderInfo(std::move(mojom_patterns),
                                 test_future.GetCallback());
    return test_future.Take();
  }

 private:
  std::unique_ptr<service_manager::BinderRegistry> registry_;
  std::unique_ptr<OrderExtractor> extractor_;
};

TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest,
       MediaMarktCheckout) {
  // Arrange
  LoadHTML(
      R"HTML(<!doctype html>
<html><head><title>mediamarkt.de checkout synthetic hierarchy fixture</title></head>
<body>
<div></div>
<div><div></div><div></div><div><div><div data-test="foo"><header><div><div><div><a aria-label="foo" data-test="foo"><div><span aria-hidden="true"><svg role="foo"><path></path><path></path><path></path><path></path><path></path></svg></span></div><img data-test="foo"/></a><div><span aria-hidden="true"><svg role="foo"><path></path></svg></span><div><span> [foo] </span></div></div></div></div></div></header></div></div><main><div></div><div><div><div><div><div><div><nav data-test="foo"><div></div><div><div><span><span data-test="foo"></span></span><ul><li data-test="foo"><a aria-label="foo"><span><span aria-hidden="true"><svg><path></path></svg></span></span><span> [foo] </span></a></li><li data-test="foo"><a aria-label="foo"><span><span aria-hidden="true"><svg><path></path></svg></span></span><span> [foo] </span></a></li><li data-test="foo"><a aria-label="foo"><span><span aria-hidden="true"><svg><path></path></svg></span></span><span> [foo] </span></a></li><li data-test="foo"><div aria-label="foo"><span><span> [foo] </span></span><span> [foo] </span></div></li></ul></div></div><div></div></nav><div><div data-test="checkout-summary-page"><div><div><aside aria-label="foo" data-test="foo"><div><div><div data-test="foo"><div><div><h2> [foo] </h2><a data-test="foo"><span aria-hidden="true"><svg role="foo"><path></path></svg></span> [foo] </a><a data-test="foo"><span aria-hidden="true"><svg role="foo"><path></path></svg></span> [foo] </a></div><div role="foo"></div></div></div><div><section><ul role="foo"><li><div></div><div><p><button><span data-test="foo"><span><div><p> [foo] </p><div><span data-test="foo">€0.42</span></div></div></span></span><span aria-hidden="true"><svg role="foo"><path></path></svg></span></button></p><div></div></div><div></div><div aria-hidden="true" data-test="foo"><div data-test="foo"><div role="foo"><div><span> [foo] </span><span><span data-test="foo">€0.42</span></span></div></div></div></div></li></ul></section><div><div><span> [foo] </span><button aria-label="foo"><span><span aria-hidden="true"><svg role="foo"><path></path></svg></span></span></button></div><span><div><span>€0.42</span></div></span></div><div></div></div><div><div><div><span data-test="foo">Gesamtsumme</span><div><span> [foo] </span></div></div><span data-test="checkout-total"><span>4,42 €</span></span></div></div><div><div><div data-test="foo"><button data-test="foo"><span><span> [foo] </span></span></button><div><span></span><span><div data-test="foo"><p> [foo] <button><span><span> [foo] </span></span></button> [foo] <button><span><span> [foo] </span></span></button> [foo] <button><span><span> [foo] </span></span></button> [foo] <a data-test="foo"><span> [foo] </span></a></p></div></span></div></div></div></div></div></div></aside><div><h1 data-test="foo"> [foo] </h1></div></div><div><div><section aria-label="foo"><ul data-test="foo"><li data-test="foo"><div><div><div><h5> [foo] </h5><div><div><span data-test="foo">John Doe</span><br aria-hidden="true"/><span data-test="foo"><span data-test="foo">Teststraße 42</span></span><br aria-hidden="true"/><span data-test="foo">42424 Fooville</span><br aria-hidden="true"/><span data-test="foo"><span>john.doe@example.com</span></span></div></div></div><div><div data-test="foo"><div><div></div><span aria-hidden="true"><svg role="foo"><path></path></svg></span></div><div><span> [foo] </span></div></div></div><div></div><div><a aria-label="foo" data-test="foo"> [foo] </a></div></div></div></li><li data-test="foo"><div><div><div><h5> [foo] </h5><div><span data-test="foo"> [foo] </span></div></div><div></div><div><button aria-label="foo"><span> [foo] </span></button></div></div></div></li></ul></section><div><h2> [foo] </h2><a data-test="foo"> [foo] </a></div><div><section aria-label="foo"><div><div><div><div><div><div><h4><div data-test="foo"><div><div><div></div></div><div><div><span> [foo] </span></div></div></div></div></h4><div><span> [foo] </span></div></div></div></div></div><div><ul><li><div><div><div><div><a data-test="foo"><h4 data-test="foo"> [foo] </h4></a></div></div></div><div><a aria-label="foo" data-test="foo"><div><picture aria-hidden="true" data-test="foo"><img/></picture></div></a></div><div><div data-test="foo"><div data-test="foo"><div></div><div><span aria-hidden="true">€0.42</span><span>€0.42</span></div><div><div></div></div></div></div></div><div><p> [foo] </p></div><div><button data-test="foo"><span><span aria-hidden="true"><svg role="foo"><path></path></svg></span><span> [foo] </span></span></button></div><div></div></div></li></ul></div></div></div></section></div></div></div><div><div><aside aria-label="foo" data-test="foo"><div><div><div data-test="foo"><div><div><h2> [foo] </h2><a data-test="foo"><span aria-hidden="true"><svg role="foo"><path></path></svg></span> [foo] </a><a data-test="foo"><span aria-hidden="true"><svg role="foo"><path></path></svg></span> [foo] </a></div><div role="foo"></div></div></div><div><section><ul role="foo"><li><div></div><div><p><button><span data-test="foo"><span><div><p> [foo] </p><div><span data-test="foo">€0.42</span></div></div></span></span><span aria-hidden="true"><svg role="foo"><path></path></svg></span></button></p><div></div></div><div></div><div aria-hidden="true" data-test="foo"><div data-test="foo"><div role="foo"><div><span> [foo] </span><span><span data-test="foo">€0.42</span></span></div></div></div></div></li></ul></section><div><div><span> [foo] </span><button aria-label="foo"><span><span aria-hidden="true"><svg role="foo"><path></path></svg></span></span></button></div><span><div><span>€0.42</span></div></span></div><div></div></div><div><div><div><span data-test="foo">Gesamtsumme</span><div><span> [foo] </span></div></div><span data-test="checkout-total"><span>4,42 €</span></span></div></div><div><div role="foo"></div><div><ul><li data-test="foo"><div><div><div><h3> [foo] </h3><button><span><span> [foo] </span></span></button></div></div></div></li></ul></div></div><div><div><div data-test="foo"><button data-test="foo"><span><span> [foo] </span></span></button><div><span></span><span><div data-test="foo"><p> [foo] <button><span><span> [foo] </span></span></button> [foo] <button><span><span> [foo] </span></span></button> [foo] <button><span><span> [foo] </span></span></button> [foo] <a data-test="foo"><span> [foo] </span></a></p></div></span></div></div></div></div></div></div></aside></div></div></div></div></div></div></div></div></div></div><div></div></div></main><div></div> [foo] <div data-test="foo"><footer></footer></div> [foo] </div><ul></ul></div>
<div></div>
<div></div>
<script></script>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->value_selectors.push_back(
      BuildSelector("[data-test=\"checkout-total\"]"));

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_THAT(mojom_order->value, ::testing::Optional(::testing::Eq("4,42")));
  EXPECT_FALSE(mojom_order->id);
}

TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest,
       MediaMarktThankYou) {
  // Arrange, order ID substituted from the captured page's repeating-digit
  // placeholder ("424242424") to a non-repeating one, so a digit-duplication
  // bug could not coincidentally still pass this assertion.
  LoadHTML(
      R"HTML(<!doctype html>
<html><head><title>mediamarkt.de thankYou synthetic hierarchy fixture</title></head>
<body>
<div></div>
<div><div></div><div></div><div><div><div data-test="foo"><div><a data-test="foo"> [foo] </a></div><header></header><section><div><ul aria-label="foo"><li><span><a data-test="foo"> [foo] </a></span></li></ul></div></section></div></div><main><div></div><div><div><div><div><div><div><div><div data-test="checkout-thankyou-page"><section aria-label="foo"><div><div><div aria-hidden="true"></div><div><div><h5> [foo] </h5><p> [foo] </p></div></div></div></section><div></div><div data-test="foo"></div><div></div><section aria-label="foo"><div><div aria-hidden="true"></div><div><div><h1> [foo] </h1></div></div><div aria-hidden="true"></div></div><div><div aria-hidden="true"></div><div><h2>Deine Bestellnummer:  305817264</h2></div><div aria-hidden="true"></div></div><div><div><div aria-hidden="true"></div><div><div><span> [foo] </span></div></div><div aria-hidden="true"></div></div></div><div><div aria-hidden="true"></div><div><div><a data-test="foo"> [foo] </a><a data-test="foo"> [foo] </a></div><div><div><div><div><section><div><div><span aria-hidden="true"><svg><path></path></svg></span><h5> [foo] </h5></div><div><div data-test="foo"><span>John Doe</span><span>Teststraße 42</span><span>42424 Fooville</span></div></div></div></section></div><div><section><div><div><span aria-hidden="true"><svg><path></path></svg></span><h5>Rechnungsadresse</h5></div><div><div data-test="foo"><span>John Doe</span><span>Teststraße 42</span><span>42424 Fooville</span></div></div></div></section></div></div></div></div></div><div><div><h3> [foo] </h3></div><div><div><div><span data-test="foo">Gesamtsumme</span><div><span> [foo] </span></div></div><span data-test="checkout-total"><span>4,42 €</span></span></div></div></div></div><div aria-hidden="true"></div></div></section></div></div></div></div></div></div></div><div></div></div></main><div></div> [foo] <div data-test="foo"><footer></footer></div> [foo] </div><ul></ul></div>
<div></div>
<div></div>
<script></script>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->value_selectors.push_back(
      BuildSelector("[data-test=\"checkout-total\"]"));
  mojom_patterns->id_regexes = {std::string(test::kDeOrderIdRegex)};

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_THAT(mojom_order->value, ::testing::Optional(::testing::Eq("4,42")));
  EXPECT_THAT(mojom_order->id, ::testing::Optional(::testing::Eq("305817264")));
}

TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest, OttoCheckout) {
  // Arrange, the grand total substituted from the captured page's
  // symmetric-digit placeholder ("42,42") to a non-symmetric one, so a
  // digit-swap bug could not coincidentally still pass this assertion; the
  // shopping-cart ID is kept as captured since it is a repeated hex word,
  // not a decimal digit sequence, so it carries no such risk.
  LoadHTML(
      R"HTML(<!doctype html>
<html><head><title>otto.de checkout synthetic hierarchy fixture</title></head>
<body data-qa="foo">
<div></div>
<div>
<main>
<div>
<div data-qa="content">
<div data-qa="grandTotal">
<div>
<h2>Gesamtsumme</h2>
<span data-qa="grandTotalAmount">13,37</span>
</div>
</div>
<div>
<oc-button-v1 data-qa="checkoutNow" data-shopping-cart-id="42c0ffee42c0ffee42c0ffee"> [foo] </oc-button-v1>
</div>
</div>
</div>
</main>
</div>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->value_selectors.push_back(
      BuildSelector("[data-qa=\"grandTotalAmount\"]"));
  mojom_patterns->id_selectors.push_back(
      BuildSelector("[data-shopping-cart-id]", "data-shopping-cart-id"));

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_THAT(mojom_order->value, ::testing::Optional(::testing::Eq("13,37")));
  EXPECT_THAT(mojom_order->id,
              ::testing::Optional(::testing::Eq("42c0ffee42c0ffee42c0ffee")));
}

TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest, OttoThankYou) {
  // Arrange, no value selector is configured for otto.de's thank-you page
  // and the total is not visible there, so value is correctly not found.
  LoadHTML(
      R"HTML(<!doctype html>
<html><head><title>otto.de thankYou synthetic hierarchy fixture</title></head>
<body data-qa="foo">
<div>
<main>
<section>
<div data-qa="foo">
<div data-qa="foo">
<div data-order-id="42c0ffee42c0ffee42c0ffee">
</div>
</div>
</section>
</main>
</div>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->id_selectors.push_back(
      BuildSelector("[data-order-id]", "data-order-id"));

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_FALSE(mojom_order->value);
  EXPECT_THAT(mojom_order->id,
              ::testing::Optional(::testing::Eq("42c0ffee42c0ffee42c0ffee")));
}

// A second, independently captured snapshot of the same page; the
// extraction-relevant markup is identical to `OttoThankYou` above.
TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest, OttoThankYouB) {
  // Arrange
  LoadHTML(
      R"HTML(<!doctype html>
<html><head><title>otto.de thankYou synthetic hierarchy fixture</title></head>
<body data-qa="foo">
<div>
<main>
<section>
<div data-qa="foo">
<div data-qa="foo">
<div data-order-id="42c0ffee42c0ffee42c0ffee">
</div>
</div>
</section>
</main>
</div>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->id_selectors.push_back(
      BuildSelector("[data-order-id]", "data-order-id"));

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_FALSE(mojom_order->value);
  EXPECT_THAT(mojom_order->id,
              ::testing::Optional(::testing::Eq("42c0ffee42c0ffee42c0ffee")));
}

TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest, SixtCheckout) {
  // Arrange, the page has a second, aria-hidden duplicate of the same
  // amount elsewhere (a collapsed trip-summary bar), which the
  // `:not([aria-hidden])` clause in the selector must correctly skip.
  LoadHTML(
      R"HTML(<!doctype html>
<html><head><title>sixt.de checkout synthetic hierarchy fixture</title></head>
<body>
<div><main><div><div data-testid="foo"><div><div><div><div><button aria-label="Preisdetails"><span><span>Gesamt:</span><span>424,24 €</span><span aria-hidden="true"><span> [foo] </span></span></span><span aria-hidden="true"><svg><path></path></svg></span></button></div></div></div></div><div><div><div><div>Gesamt</div><button aria-label="Preisdetails"><span><span>424,24 €</span><span aria-hidden="true"><span> [foo] </span></span></span><span aria-hidden="true"><svg><path></path></svg></span></button></div></div></div></div></div></main></div>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->value_selectors.push_back(
      BuildSelector("button[aria-label=\"Preisdetails\"] > span > "
                    "span:nth-of-type(2):not([aria-hidden])"));

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_THAT(mojom_order->value, ::testing::Optional(::testing::Eq("424,24")));
  EXPECT_FALSE(mojom_order->id);
}

TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest, SixtThankYou) {
  // Arrange, no value selector is configured for sixt.de's thank-you page,
  // so the generic heuristic finds it; this page prices the booking in
  // USD, not EUR, a genuine per-booking difference, not a bug. The booking
  // reference, repeated three times in the page's own markup, is
  // substituted from the captured page's repeating-digit placeholder
  // ("4242424242") to a non-repeating one in all three places, so a
  // digit-duplication bug could not coincidentally still pass this
  // assertion.
  LoadHTML(
      R"HTML(<!doctype html>
<html><head><title>sixt.de thankYou synthetic hierarchy fixture</title></head>
<body>
<div><main><div><div><div data-testid="foo"><div data-testid="foo"><div><div><div><div><span data-testid="foo"></span><div data-testid="foo"> [foo] </div></div><div data-testid="foo"><span><span><div><span aria-label="foo" role="button"><div>9305817264</div><span aria-hidden="true"><svg><path></path></svg></span></span></div></span></span></div></div></div></div></div></div><div data-testid="foo"><div><div><div data-testid="foo"><div data-testid="foo"><div><div><div><span data-testid="foo"></span><div data-testid="foo"> [foo] </div></div><div data-testid="foo"><span><span><div><span aria-label="foo" role="button"><div>9305817264</div><span aria-hidden="true"><svg><path></path></svg></span></span></div></span></span></div></div></div></div></div></div><div><div><div><div><div><h2> [foo] </h2></div></div><div><div><div><div><div><div>Gesamt (inkl. MwSt.)</div></div><div><div><span><span>424,24 $</span></span></div></div></div></div></div></div></div></div><div><div><div data-testid="foo"><div><h2 data-testid="foo"> [foo] </h2><div data-testid="mysixt-bookings-booking-actions-reference-number"><div data-testid="foo"> [foo] </div><span><span><div><span role="button"><div>9305817264</div><span aria-hidden="true"><svg><path></path></svg></span></span></div></span></span></div></div></div></div></div></main></div>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->value_keyword_regexes = {
      std::string(test::kDeOrderValueKeywordRegex)};
  mojom_patterns->value_currency_regexes = {
      {"USD", std::string(test::kUsdOrderValueCurrencyRegex)}};
  mojom_patterns->number_of_ancestor_levels_to_search = 6;
  mojom_patterns->id_selectors.push_back(BuildSelector(
      "[data-testid=\"mysixt-bookings-booking-actions-reference-number\"] "
      "[role=\"button\"] div"));

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_THAT(mojom_order->value, ::testing::Optional(::testing::Eq("424,24")));
  EXPECT_THAT(mojom_order->id,
              ::testing::Optional(::testing::Eq("9305817264")));
}

// A second capture of the same page with a popover's markup present in
// the DOM (extra unrelated buttons appended after the real content), to
// confirm that noise doesn't confuse the extraction.
TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest,
       SixtThankYouPopover) {
  // Arrange
  LoadHTML(
      R"HTML(<!doctype html>
<html><head><title>sixt.de thankYou synthetic hierarchy fixture</title></head>
<body>
<div><main><div><div><div data-testid="foo"><div data-testid="foo"><div><div><div><div><span data-testid="foo"></span><div data-testid="foo"> [foo] </div></div><div data-testid="foo"><span><span><div><span aria-label="foo" role="button"><div>9305817264</div><span aria-hidden="true"><svg><path></path></svg></span></span></div></span></span></div></div></div></div></div></div><div data-testid="foo"><div><div><div data-testid="foo"><div data-testid="foo"><div><div><div><span data-testid="foo"></span><div data-testid="foo"> [foo] </div></div><div data-testid="foo"><span><span><div><span aria-label="foo" role="button"><div>9305817264</div><span aria-hidden="true"><svg><path></path></svg></span></span></div></span></span></div></div></div></div></div></div><div><div><div><div><div><h2> [foo] </h2></div></div><div><div><div><div><div><div>Gesamt (inkl. MwSt.)</div></div><div><div><span><span>424,24 $</span></span></div></div></div></div></div></div></div></div><div><div><div data-testid="foo"><div><h2 data-testid="foo"> [foo] </h2><div data-testid="mysixt-bookings-booking-actions-reference-number"><div data-testid="foo"> [foo] </div><span><span><div><span role="button"><div>9305817264</div><span aria-hidden="true"><svg><path></path></svg></span></span></div></span></span></div></div></div></div><div aria-label="foo" role="foo"><button aria-label="foo"></button><button aria-label="foo"></button><button aria-label="foo"></button></div></div></main></div>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->value_keyword_regexes = {
      std::string(test::kDeOrderValueKeywordRegex)};
  mojom_patterns->value_currency_regexes = {
      {"USD", std::string(test::kUsdOrderValueCurrencyRegex)}};
  mojom_patterns->number_of_ancestor_levels_to_search = 6;
  mojom_patterns->id_selectors.push_back(BuildSelector(
      "[data-testid=\"mysixt-bookings-booking-actions-reference-number\"] "
      "[role=\"button\"] div"));

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_THAT(mojom_order->value, ::testing::Optional(::testing::Eq("424,24")));
  EXPECT_THAT(mojom_order->id,
              ::testing::Optional(::testing::Eq("9305817264")));
}

TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest, ZalandoCheckout) {
  // Arrange, no value selector is configured for zalando.de, so the
  // generic heuristic finds it; no ID is visible on this page. The two
  // real captures of this page had identical extraction-relevant markup,
  // so only one is kept here. The total is substituted from the captured
  // page's symmetric-digit placeholder ("42,42") to a non-symmetric one, so
  // a digit-swap bug could not coincidentally still pass this assertion.
  LoadHTML(
      R"HTML(<!doctype html>
<html><head><title>zalando.de checkout synthetic hierarchy fixture</title></head>
<body><div><div><div><div><h1>Bestellübersicht</h1></div></div><div><div><div><div><span> [foo] </span></div><div><span>€0.42</span></div></div><div><div><div><span>Gesamtsumme</span><span> [foo] </span></div><div><span>24,99 €</span></div></div></div></div></div></div></div>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->value_keyword_regexes = {
      std::string(test::kDeOrderValueKeywordRegex)};
  mojom_patterns->value_currency_regexes = {
      {"EUR", std::string(test::kEurOrderValueCurrencyRegex)}};
  mojom_patterns->number_of_ancestor_levels_to_search = 6;

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_THAT(mojom_order->value, ::testing::Optional(::testing::Eq("24,99")));
  EXPECT_FALSE(mojom_order->id);
}

TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest, ZalandoThankYou) {
  // Arrange, the order total sits in an unrelated sibling with no nearby
  // label, so value is correctly not found here (it was only visible on
  // the earlier checkout page); the order ID resolves via selector. The
  // two real captures of this page had identical extraction-relevant
  // markup, so only one is kept here. The total is kept consistent with
  // the checkout page's order ("24,99 €", same order), and the order ID is
  // substituted from the captured page's symmetric-digit placeholder
  // ("42424242424242") to a non-repeating one, so a digit-duplication bug
  // could not coincidentally still pass this assertion.
  LoadHTML(
      R"HTML(<!doctype html>
<html><head><title>zalando.de thankYou synthetic hierarchy fixture</title></head>
<body><div><div><div><div><div><div><div><div><div><div><div><span>24,99 €</span></div><div><span data-testid="orderNumber">90210458317</span></div></div></div></div></div></div></div></div></div></div>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->value_keyword_regexes = {
      std::string(test::kDeOrderValueKeywordRegex)};
  mojom_patterns->value_currency_regexes = {
      {"EUR", std::string(test::kEurOrderValueCurrencyRegex)}};
  mojom_patterns->number_of_ancestor_levels_to_search = 6;
  mojom_patterns->id_selectors.push_back(
      BuildSelector("[data-testid=\"orderNumber\"]"));

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_FALSE(mojom_order->value);
  EXPECT_THAT(mojom_order->id,
              ::testing::Optional(::testing::Eq("90210458317")));
}

TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest, RebikeThankYou) {
  // Arrange, no value selector is configured for rebike.com by domain in
  // this test (exercised separately below); the ID has no selector and is
  // found via the generic DE regex instead. The value is substituted from
  // the captured page's symmetric-digit placeholder ("42,42") to a
  // non-symmetric one, and the confirmation number from an alphanumeric
  // placeholder containing digits ("FOO42BAR") to a purely alphabetic
  // metasyntactic one, so neither a digit-swap nor digit-duplication bug
  // could coincidentally still pass these assertions.
  LoadHTML(
      R"HTML(<!doctype html>
<html><head><title>rebike.com thankYou synthetic hierarchy fixture</title></head><body>
<div><div><div><div><a> [foo] </a><div><div></div><aside data-inspector-id="orderSummary"><div><div><button><span><span>Bestellübersicht<span><svg aria-hidden="true"><use></use></svg></span></span><span><span>Gesamtpreis</span><span><p><strong>19,95 €</strong></p></span></span></span></button><div><div></div></div></div></div></aside><div><div><div><main><h1> [foo] </h1><div><div><div><header><div><p>Bestätigungsnr. FOOBARBAZ</p><h2> [foo] </h2></div></header><div><div><div><section aria-label="foo"><div><div><div><div><span aria-label="foo" role="foo"><span> [foo] </span></span></div></div></div></section></div></div></div></div></div></main></div></div></div></div></div></div></div>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->value_selectors.push_back(
      BuildSelector("aside[data-inspector-id=\"orderSummary\"] strong"));
  mojom_patterns->id_regexes = {std::string(test::kDeOrderIdRegex)};

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_THAT(mojom_order->value, ::testing::Optional(::testing::Eq("19,95")));
  EXPECT_THAT(mojom_order->id, ::testing::Optional(::testing::Eq("FOOBARBAZ")));
}

TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest, BookingThankYou) {
  // Arrange, label and value share a single text node ("Total price:
  // US$88.17"), the combined-text-node case this feature already handled
  // unchanged; the ID is split across sibling elements, the element-text
  // fallback case. Both the value and ID are substituted from the captured
  // page's symmetric/repeating-digit placeholders ("42.42", "4242424242")
  // to non-symmetric, non-repeating ones, so a digit-swap or
  // digit-duplication bug could not coincidentally still pass these
  // assertions.
  LoadHTML(
      R"HTML(<!doctype html>
<html><head><title>booking.com thankYou synthetic hierarchy fixture</title></head>
<body>
<div><main><div><div data-testid="foo"><div><div><div><div><div data-testid="foo"><div><div><div><div><div><div><div data-testid="ConfirmationCard"><h2> [foo] </h2><div><div role="button"><div><div><div><div><picture><img/></picture></div></div><div><h3><div> [foo] </div></h3><ul><span role="listitem"><div><div><div><div>Total price: US$88.17</div></div></div></div></span></ul></div></div></div></div></div></div></div></div></div></div><div><div data-testid="foo"><div><div><div><div><div><div data-testid="bn_pin_box"><div><span>Confirmation number:</span><span>77318495026</span></div></div></div></div></div></div></div></div></div></main></div>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->value_keyword_regexes = {R"RE2((?i)^Total price:)RE2"};
  mojom_patterns->value_currency_regexes = {
      {"USD", R"RE2((?:US\$|USD|\$)\s*\d+\.\d{2})RE2"}};
  mojom_patterns->number_of_ancestor_levels_to_search = 6;
  mojom_patterns->id_regexes = {std::string(test::kEnOrderIdRegex)};

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_THAT(mojom_order->value, ::testing::Optional(::testing::Eq("88.17")));
  EXPECT_THAT(mojom_order->id,
              ::testing::Optional(::testing::Eq("77318495026")));
}

TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest,
       ExpressVpnCheckout) {
  // Arrange, no ID is visible on this page.
  LoadHTML(
      R"HTML(<!doctype html>
<html lang="en">
<head><title>FooVPN checkout synthetic full hierarchy test</title></head>
<body class="inter_c98ef1dc-module__yQlASG__className fskim_8a9d2058-module__NxKUPa__variable antialiased w-full" style="padding-top: 54px;">
<div><!--$--><!--/$--></div><div></div><!--$--><!--/$--><script></script><div><div><div></div><input/></div></div>
<div><header><div><span></span></div><div><!--$--><button><svg><circle></circle><path></path><path></path></svg><span>[foo]</span><span>[foo]</span></button><!--/$--></div></header>
<div><div><div><div><div><div><div><img/><p><b>[foo]</b><span></span><span>[foo]</span></p></div><div><h4>[foo]</h4><div><label>[foo]</label><input/></div></div><div><p>[foo]<button>[foo]</button>[foo]<a>[foo]</a>[foo]</p></div></div></div>
<div><button><p>[foo]</p><div><img/><div>[foo]</div></div></button><button><span></span></button><button><img/><img/></button><button><img/></button></div>
<div><div><h3><button><div><p>[foo]</p><div><div><span><img/></span><span><img/></span><span><img/></span><span><img/></span><span><img/></span><span><img/></span><span><img/></span><span><div>[foo]</div></span></div></div></div><svg><path></path></svg></button></h3>
<div><div><div><div><div><input/></div></div><div><iframe></iframe><div><div><div><div><iframe></iframe><svg><rect></rect><line></line></svg></div><div><div><iframe></iframe></div><div><iframe></iframe><svg><circle></circle><path></path><path></path></svg></div></div></div></div>
<div><div><p>[foo]</p><p>[foo]<a>[foo]</a>[foo]<a>[foo]</a>[foo]<a>[foo]</a>[foo]</p><p><b>[foo]</b>[foo]<b>[foo]</b>[foo]<b>[foo]</b>[foo]<span>[foo]</span>[foo]<b>[foo]</b>[foo]</p></div><div><div><button>[foo]</button></div></div></div></div></div></div></div></div></div>
<div><h3><button><span></span><svg><path></path></svg></button></h3><div><div><div><div><div><p>[foo]</p><p>[foo]<a>[foo]</a>[foo]<a>[foo]</a>[foo]<a>[foo]</a>[foo]</p><p><b>[foo]</b>[foo]<b>[foo]</b>[foo]<b>[foo]</b>[foo]<span>[foo]</span>[foo]<b>[foo]</b>[foo]</p></div><div><div></div><div><apple-pay-button></apple-pay-button></div></div></div></div></div></div></div>
<div><h3><button><img/><img/><svg><path></path></svg></button></h3><div><div><div><div><div><p>[foo]</p><p>[foo]<a>[foo]</a>[foo]<a>[foo]</a>[foo]<a>[foo]</a>[foo]</p><p><b>[foo]</b>[foo]<b>[foo]</b>[foo]<b>[foo]</b>[foo]<span>[foo]</span>[foo]<b>[foo]</b>[foo]</p></div><div><div></div><div><div><div><style></style><iframe></iframe><div></div><div></div><iframe></iframe></div></div></div></div></div></div></div></div></div>
<div><h3><button><img/><svg><path></path></svg></button></h3><div><div><div><div><div><p>[foo]</p><p>[foo]<a>[foo]</a>[foo]<a>[foo]</a>[foo]<a>[foo]</a>[foo]</p><p><b>[foo]</b>[foo]<b>[foo]</b>[foo]<b>[foo]</b>[foo]<span>[foo]</span>[foo]<b>[foo]</b>[foo]</p></div><div><div></div><div><div><div><button></button></div></div></div></div></div></div></div></div></div></div></div></div>
<div><div><div><div><div><div><svg><path></path><path></path></svg><p>[foo]</p></div></div></div>
<div><div><h4>Order summary</h4></div>
<div><div><span><span></span></span><p>FooVPN Advanced</p></div>
<div><span><img/></span><span><img/></span><span><img/></span><span><img/></span><span><img/></span><span><img/></span><span><img/></span></div>
<div><div><div>[foo]</div><div>[foo]</div></div></div>
<div><div><div><div><p>VPN plan</p></div><div><p><span>$142.42</span><sup>[foo]</sup></p><p>$13.37</p></div></div><p><span>[foo]</span>[foo]</p></div>
<div></div>
<div><div><p>Subtotal</p><p>$13.37</p></div><div><p>Sales tax/VAT 0%</p><p>$0.00</p></div><div><span>Country:</span><button>Fooville</button></div></div>
<div class="flex flex-col"><div class="flex items-center justify-between h-7"><h4 class="text-lg font-bold">Total</h4><p class="text-mint dark:text-peppermint text-lg font-bold">$13.37</p></div></div>
<button>[foo]</button></div></div></div></div>
<div><div><div><h6>[foo]</h6><img/><img/></div><p>[foo]</p></div></div><div><p>[foo]</p><img/></div></div></div></div></div>
<footer><div><p>[foo]</p><div><a>[foo]</a><a>[foo]</a><a>[foo]</a><a>[foo]</a></div></div></footer>
</div><!--$--><!--/$-->
<script></script><script src="data:text/javascript,"></script>
<script></script><script></script><script></script>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->value_keyword_regexes = {
      std::string(test::kEnOrderValueKeywordRegex)};
  mojom_patterns->value_currency_regexes = {
      {"USD", std::string(test::kUsdOrderValueCurrencyRegex)}};
  mojom_patterns->number_of_ancestor_levels_to_search = 6;

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_THAT(mojom_order->value, ::testing::Optional(::testing::Eq("13.37")));
  EXPECT_FALSE(mojom_order->id);
}

TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest,
       ExpressVpnThankYou) {
  // Arrange, the order ID is embedded in a Next.js RSC payload inside an
  // inline script, preceded by several empty inline scripts (as the real
  // page has, dozens of unrelated ones), so this also regression-tests
  // trying every matching element, not just the first; no value is
  // visible on this page. The ID is substituted from the captured page's
  // symmetric-digit placeholder ("42424242-4242-4242-4242-424242424242")
  // to a non-repeating hex word, so a digit-duplication bug could not
  // coincidentally still pass this assertion.
  LoadHTML(
      R"HTML(<!doctype html>
<html lang="en">
<head><title>FooVPN welcome synthetic full hierarchy test</title></head>
<body class="font-body antialiased">
<div></div><div></div><!--$--><!--/$--><section></section><!--$--><!--/$-->
<div><a>[foo]</a>
<header><a><svg></svg></a><span><svg></svg><span>[foo]</span></span></header>
<nav><div><p>[foo]</p>
<div><div><div></div><ol>
<li><div><div><svg></svg></div><span>[foo]</span></div><div><div><div></div></div></div></li>
</ol><div></div></div></div>
</div></nav>
<main><!--$--><div><div><div><div>
<div><div><!--$--><div><nav><div><h4>[foo]</h4>
<ul><li><div><button><span>[foo]</span><svg></svg></button>
<div><ul>
<li><button><span><svg></svg></span><span>[foo]</span></button></li>
</ul></div></div></li>
</ul></div>
</nav></div>
<div><div>
<section><div><div><div><h4>[foo]</h4><p>[foo]</p></div></div></div><div><button>[foo]</button></div></section>
<div><p>[foo]</p></div>
</div></div><!--/$--></div></div></div></div>
<div><a>[foo]</a></div>
</div></div></div><!--/$--></main>
<!--$--><!--/$--><footer><p>[foo]</p></footer>
</div><!--$--><!--/$-->
<script></script><script src="data:text/javascript,"></script>
<script></script><script></script>
<script>
self.__next_f = self.__next_f || [];
self.__next_f.push([1, '\"subscriptionId\":\"xvpn:sub:deadbeef-dead-beef-dead-beefdeadbeef\"']);
</script>
<script></script><script></script>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->id_selectors.push_back(BuildSelector(
      "script:not([src])", /*attribute_name=*/std::nullopt,
      R"RE2((?i)xvpn:sub:([0-9a-f]{8}(?:-[0-9a-f]{4}){3}-[0-9a-f]{12}))RE2"));

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_FALSE(mojom_order->value);
  EXPECT_THAT(mojom_order->id, ::testing::Optional(::testing::Eq(
                                   "deadbeef-dead-beef-dead-beefdeadbeef")));
}

TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest, BestBuyThankYou) {
  // Arrange, the total is read via this domain's configured script-plus-
  // pattern selector, not visible text, with several empty inline scripts
  // before the real one, so every matching element is tried, not just the
  // first.
  LoadHTML(
      R"HTML(<!doctype html>
<html><head><title>bestbuy.com thankYou synthetic hierarchy fixture</title></head>
<body class="bg-default-subdued text-default" data-app-ver="foo">
<div id="thankyou-container">
<div id="thankyou-header" style="min-height: 132px"> [foo] </div>
<div>
<div id="thankyouApp" class="brix" data-bundle-file="foo" style="min-height: 100vh"><main class="bg-default-subdued"><div class="m-auto flex flex-col gap-100 lg__gap-200 max-md__max-w-screen-sm md__max-w-screen-md lg__max-w-screen-lg xl__max-w-screen-xl lg__px-300 md__mt-0"><div class="checkout-digital-atwb display-ad-pencil-banner"><div> [foo] </div></div><div class="checkout-digital-atwb ninja-component"><div> [foo] </div></div><div class="bg-default self-center w-full lg__rounded-xl"><div class="px-200 md__px-300 py-300"><div class="flex flex-row w-full"><div class="w-full lg__w-1/2"><h1 id="confirmationCardTitle" data-testid="ThankYouPageTitle" class="flex gap-100 items-center mb-200"><svg xmlns="http://www.w3.org/2000/svg" aria-hidden="true" role="img" class="fill text-success" width="24" height="24"><path></path></svg><span class="font-sans text-default text-style-title-lg-500"> [foo] </span></h1><div class="flex flex-col gap-200"><div><span class="font-sans text-style-body-md-400 text-default-muted"> [foo] </span><span class="font-sans text-default text-style-body-md-500"> [foo] </span></div><div class="flex flex-col gap-50"><button data-brix-component-name="TextButton" data-brix-version="1.81.3" data-track="foo" role="button" aria-busy="false" class="relative bg-transparent border-none rounded-sm p-0 cursor-pointer inline-flex flex-col text-style-body-md-500 w-fit"><div style="position: absolute; inset: -3px;"></div><div data-brix-component-name="Row" data-brix-version="1.81.3" class="flex flex-row justify-start items-center gap-50"><span class="font-sans text-style-body-md-500 text-primary"> [foo] </span></div></button><span class="font-sans text-style-body-sm-400 text-default-muted">Order #: BBY03-507281924617</span></div><div class="mt-100 undefined"><div data-brix-component-name="Divider" data-brix-version="1.81.3" aria-hidden="true" class="border-comp-divider-default self-stretch border-b-xs border-t-none border-l-none border-r-none border-solid"></div><div class="pt-300"><div data-testid="shipping-title" class="flex flex-col gap-50"><span class="font-sans text-default text-style-body-lg-500"> [foo] </span><span class="font-sans text-style-body-sm-400 line-clamp text-default-muted"> [foo] </span></div><ul class="p-0 m-0"><li class="list-none"><div class="py-300 flex flex-row gap-300"><div class="w-1/2 flex flex-col gap-100"><div data-testid="ShippingItemsHeading" class="flex flex-col"><span class="font-sans text-style-body-md-500 text-success"> [foo] </span></div></div><div class="w-1/2 flex flex-col gap-100"><div class="flex flex-row gap-200 flex-wrap"><div data-testid="TypItemImageContainer-0" class="relative w-800 h-800 flex justify-content-center align-items-center"><div class="w-700 h-700 shrink-0"><img/></div><div class="absolute top-0 bottom-0 w-full bg-brand-light-gray opacity-10 rounded-lg"></div></div></div></div></div></li></ul></div></div><div class="hidden"></div></div></div></div></div></div></div><div class="bg-default self-center w-full lg__rounded-xl"><div class="px-200 md__px-300 py-300"> [foo] </div></div><div class="checkout-digital-atwb display-ad-rokt"><div> [foo] </div></div><div class="customer-feedback" id="thank-you-customer-feedback"> [foo] </div></div></main></div>
</div>
<div id="thankyou-footer" style="min-height: 252px"> [foo] </div>
</div>
<script></script><script src="data:text/javascript,"></script><script></script>
<script type="text/javascript">
var orderData = {"id":"deadbeef-dead-beef-dead-beefdeadbeef","userOrderId":"BBY03-507281924617","price":{"productTotal":233.99,"totalSalesTax":24.57,"orderTotal":258.56}};
</script>
<script></script><script></script>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->value_selectors.push_back(BuildSelector(
      "script:not([src])", /*attribute_name=*/std::nullopt,
      R"RE2((?i)"orderTotal"\s*:\s*"?([0-9]+(?:\.[0-9]{2})?)"?)RE2"));
  mojom_patterns->id_regexes = {std::string(test::kEnOrderIdRegex)};

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_THAT(mojom_order->value, ::testing::Optional(::testing::Eq("258.56")));
  EXPECT_THAT(mojom_order->id,
              ::testing::Optional(::testing::Eq("BBY03-507281924617")));
}

TEST_F(BraveAdsOrderExtractorRealSiteFixturesRenderViewTest, CerakeyThankYou) {
  // Arrange, the total sits in its own cost-summary table row, matched via
  // the generic keyword heuristic rather than a selector; the confirmation
  // number's repeating-digit placeholder is replaced with a real one.
  LoadHTML(
      R"HTML(<!doctype html>
<html lang="en-US" dir="ltr"><head><title>cerakey.com thankYou synthetic hierarchy fixture</title></head>
<body class="">
<div id="app"><div id="CustomProperties-P0-0"><div class="g9gqqf1 g9gqqf0 _1fragemyi _1fragemzl _1fragemyw _1fragemy7 _1fragem136"><div class="_1fragem5u _1fragemws"><div class="_1fragem5u _1fragemws"><a href="#checkout-main" class="_9sntZ"> [foo] </a><div class="cm5pp vdmiK szW4X g9PS6 U3Rye FeQiM oYrwu"><header data-inspector-id="header" class="EAjaz _3Ebqn _8wrz5 d5pfT _1fragem13b _1fragemy7 _1fragemy1"> [foo] </header><aside data-inspector-id="orderSummary" class="nMPKH iYA3J _1fragemy7 _1fragemxz _1fragem13b"><div><div><button type="button" aria-busy="false" aria-controls="disclosure_details" aria-expanded="false" data-event-name="order_summary_show" class="_1s82kpb1 _1s82kpb0 WtpiW"><span class="r0qqvk1 r0qqvk0 ik40Z"> [foo] </span></button><div id="disclosure_details" hidden="" class="_94sxtb1 _94sxtb0" style="height: 0px;"><div></div></div></div></div></aside><div class="Sxi8I"><div class="_9F1Rf _1fragemy7 _1fragemxz _1fragem13b"><div class="gdtca"><header data-inspector-id="header" class="EAjaz _3Ebqn Xx7bI"> [foo] </header><main id="checkout-main" class="djSdi"><h1 class="_1fragem124"> [foo] </h1><div class="_1fragem5u _1fragemws"><div class="_1fragempf _1fragempg _1fragemqw _1fragemsm _1fragemsn _1fragemsp _1fragemnm _1fragemnn _1fragemp3 _1fragemuf _1fragemug _1fragemui _1fragem5u _1fragemws _4Qmfj PXaZz W87Hc"><div class="r0qqvk1 r0qqvk0"><header class="r0qqvk1 r0qqvk0 DTABr"><div class="_1fragem5u _1fragemws _16s97g73h _16s97g73r GrCfj"><svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 50 50"><path></path></svg></div><div class="_1fragem5u _1fragemws"><p class="_1tx8jg70 _1fragemws">Confirmation #FOO507281</p><h2 tabindex="-1" class="n8k95w1 n8k95w0 _1fragemws n8k95w2 n8k95wg">Thank you, John!</h2></div></header><div class="_17kya4ui _17kya4ug _1fragemy7 _17kya4ul _1fragem13b _1fragem5u _1fragemws apn1wdc apn1wd4 _1fragemy3"><div class="r0qqvk1 r0qqvk0"><div class="r0qqvk1 r0qqvk0 kBVzA"><section aria-label="Your order is confirmed" class="_17kya4uj _17kya4uh sEWAT"><div class="_1fragem5u _1fragemws Q3L2z"><div class="_1h3po421 _1h3po425"><img loading="lazy" alt="foo" class="_1h3po427"></div><div aria-hidden="true" class="_17kya4u19 _4wcxL"> [foo] </div></div><div class="r0qqvk1 r0qqvk0"><h2 class="n8k95w1 n8k95w0 _1fragemws n8k95w3 n8k95wg"> [foo] </h2><div class="r0qqvk1 r0qqvk0"><p class="_1tx8jg70 _1fragemws"> [foo] </p></div></div><div class="_17kya4uj _17kya4uh _1fragem5u _1fragemws"> [foo] </div></section><div class="CNQuA"></div><section aria-label="Order details" class="_17kya4uj _17kya4uh apn1wdc apn1wd4 _1fragemy3"><div class="r0qqvk1 r0qqvk0"><div class="_1mrl40q0 _1fragemws"><h2 class="n8k95w1 n8k95w0 _1fragemws n8k95w3 n8k95wg"> [foo] </h2></div><div class="_1mrl40q0 _1fragemws"><section class="_1fragem5u _1fragemws"> [foo] </section><section class="_1fragem5u _1fragemws"> [foo] </section></div></div></section></div></div><div class="MV9Am qgQOk"> [foo] </div></div></div></div></main><footer data-inspector-id="footer" class="NGRNe"> [foo] </footer></div></div><div data-inspector-id="orderSummary" class="i4DWM _1fragemy7 _1fragemy0 _1fragem13b"><div class="_4QenE"><header data-inspector-id="header" class="EAjaz _3Ebqn oZoJd"> [foo] </header><aside><div><div><section class="_17kya4ui _17kya4ug apn1wde apn1wd6 _1fragemy5"><div class="r0qqvk1 r0qqvk0"><h2 class="n8k95wh _1fragem124"> [foo] </h2><div class="r0qqvk1 r0qqvk0"><section class="_1fragem5u _1fragemws"> [foo] </section><section class="_1fragem5u _1fragemws"><div class="nfgb6p4 _1fragem124"> [foo] </div><div role="table"><div role="rowgroup" class="nfgb6p4 _1fragem124"> [foo] </div><div role="rowgroup" class="nfgb6p3 nfgb6p2 _1fragem6t nfgb6p5"><div role="row" class="_1qy6ue60 _1qy6ue65 nfgb6p7"> [foo] </div><div role="row" class="_1qy6ue60 _1qy6ue65 nfgb6p7"> [foo] </div><div role="row" class="_1x41w3p1 _1x41w3p0 _1fragem6t _1fragem19 _1fragemxq nfgb6p7"><div role="rowheader" class="_1x41w3p2"><strong class="_19gi7yt0 _19gi7ytq _19gi7ytj _1fragemyy _19gi7yt18 _19gi7yt1g _19gi7yt1s _19gi7yt1k _1fragemz1 _1fragem69">Total</strong></div><div role="cell" class="_1x41w3p3"><div class="r0qqvk1 r0qqvk0"><abbr class="_1qifbzv1 _1qifbzv0 _1fragem12b"><span class="_19gi7yt0 _19gi7ytn"> [foo] </span></abbr><strong class="_19gi7yt0 _19gi7ytq"><span class="_1sg44lm4 _1sg44lm3">$145.20</span></strong></div></div></div><div role="row" class="_1x41w3p1 _1x41w3p0 _1fragem6t _1fragem19 _1fragemxq nfgb6pa"> [foo] </div></div></div></section></div></div></section></div></div></aside></div></div></div></div></div><div style="opacity:0;" aria-hidden="true" class="_1fragem12o"> [foo] </div><div id="PortalHost"> [foo] </div></div></div><div aria-hidden="true" class="_1rnsrjf0 _1fragem6y"></div></div>
</body></html>)HTML");

  mojom::OrderPatternsInfoPtr mojom_patterns = mojom::OrderPatternsInfo::New();
  mojom_patterns->value_keyword_regexes = {
      std::string(test::kEnOrderValueKeywordRegex)};
  mojom_patterns->value_currency_regexes = {
      {"USD", std::string(test::kUsdOrderValueCurrencyRegex)}};
  mojom_patterns->number_of_ancestor_levels_to_search = 6;
  mojom_patterns->id_regexes = {std::string(test::kEnOrderIdRegex)};

  // Act
  const mojom::OrderInfoPtr mojom_order =
      ExtractOrderInfo(std::move(mojom_patterns));

  // Assert
  EXPECT_THAT(mojom_order->value, ::testing::Optional(::testing::Eq("145.20")));
  EXPECT_THAT(mojom_order->id, ::testing::Optional(::testing::Eq("FOO507281")));
}

}  // namespace brave_ads
