/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_ORDER_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_ORDER_FETCHER_H_

#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace content {
class WebContents;
}  // namespace content

namespace brave_ads {

struct FetchState;

// Asks the main frame's `OrderExtractor` for the current page's order
// ID/value, so `AdsTabHelper` does not need its own mojo plumbing.
class OrderFetcher final {
 public:
  explicit OrderFetcher(content::WebContents& web_contents);
  ~OrderFetcher();

  OrderFetcher(const OrderFetcher&) = delete;
  OrderFetcher& operator=(const OrderFetcher&) = delete;

  using FetchOrderCallback = mojom::OrderExtractor::ExtractOrderInfoCallback;

  // Scaled up from Chromium's own `kAmountExtractionWaitTime`
  // (components/autofill/core/browser/payments/amount_extraction_manager.h),
  // since `ExtractOrderInfo` does strictly more work per call (selector
  // resolution and ID regex matching, on top of the equivalent
  // ancestor-search value matching).
  static constexpr base::TimeDelta kFetchOrderTimeout = base::Milliseconds(300);

  // Always runs `callback` exactly once. Runs an empty `OrderInfo`
  // immediately if the main frame's renderer is not live, disconnects
  // before replying, or does not reply within `kFetchOrderTimeout`.
  //
  // A still-pending fetch from an earlier call is superseded, not left to
  // race this one. Its `callback` is run with an empty `OrderInfo`
  // immediately, since any reply it later receives is for a page this
  // `OrderFetcher` no longer cares about.
  void FetchOrder(mojom::OrderExtractionPatternsInfoPtr mojom_patterns,
                  FetchOrderCallback callback);

 private:
  const raw_ref<content::WebContents> web_contents_;

  // The fetch started by the most recent `FetchOrder` call, if it hasn't
  // resolved yet.
  scoped_refptr<FetchState> active_fetch_state_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_ORDER_FETCHER_H_
