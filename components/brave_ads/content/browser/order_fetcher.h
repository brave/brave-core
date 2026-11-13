/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_ORDER_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_ORDER_FETCHER_H_

#include "base/cancelable_callback.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace content {
class WebContents;
}  // namespace content

namespace brave_ads {

class OrderFetchHelper;

using FetchOrderCallback = base::OnceCallback<void(
    bool should_extract_id,
    bool should_extract_value,
    mojom::ExtractedOrderInfoPtr mojom_extracted_order)>;

// Fetches order information from a web page using the specified extraction
// patterns.
class OrderFetcher final {
 public:
  explicit OrderFetcher(content::WebContents& web_contents);

  OrderFetcher(const OrderFetcher&) = delete;
  OrderFetcher& operator=(const OrderFetcher&) = delete;

  ~OrderFetcher();

  void FetchOrder(const mojom::OrderExtractionPatternsInfo& mojom_patterns,
                  FetchOrderCallback callback);

 private:
  const raw_ref<content::WebContents> web_contents_;

  // Helper responsible for performing the actual fetch.
  scoped_refptr<OrderFetchHelper> order_fetch_helper_;

  // Cancelable callback for the current fetch.
  base::CancelableOnceCallback<void(mojom::ExtractedOrderInfoPtr)> callback_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_ORDER_FETCHER_H_
