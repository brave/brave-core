/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_ORDER_FETCH_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_ORDER_FETCH_HELPER_H_

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace brave_ads {

// Helper class to fetch order information from a render frame host using
// the OrderExtractor mojo interface, with support for timeout handling.
// Owned by `OrderFetcher`; destroying it abandons any in-flight fetch.
class OrderFetchHelper final {
 public:
  OrderFetchHelper();

  OrderFetchHelper(const OrderFetchHelper&) = delete;
  OrderFetchHelper& operator=(const OrderFetchHelper&) = delete;

  ~OrderFetchHelper();

  // Fetches order information from the given render frame host.
  void FetchOrder(
      content::RenderFrameHost& primary_render_frame_host,
      mojom::OrderPatternsInfoPtr mojom_order_patterns,
      base::TimeDelta timeout,
      base::OnceCallback<void(mojom::ExtractedOrderInfoPtr)> callback);

 private:
  void ExtractOrderCallback(mojom::ExtractedOrderInfoPtr mojom_extracted_order);
  void TimeoutCallback();

  base::OnceCallback<void(mojom::ExtractedOrderInfoPtr)> callback_;
  mojo::Remote<mojom::OrderExtractor> order_extractor_remote_;
  base::OneShotTimer timer_;

  base::WeakPtrFactory<OrderFetchHelper> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_ORDER_FETCH_HELPER_H_
