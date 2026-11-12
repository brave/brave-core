/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_EXTRACTOR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_EXTRACTOR_H_

#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace brave_ads {

// Reads order value/ID off a checkout page's DOM for conversion reporting,
// which needs renderer process access brave_ads does not otherwise have.
class OrderExtractor final : public content::RenderFrameObserver,
                             public mojom::OrderExtractor {
 public:
  OrderExtractor(content::RenderFrame* render_frame,
                 service_manager::BinderRegistry* registry);
  ~OrderExtractor() override;

  OrderExtractor(const OrderExtractor&) = delete;
  OrderExtractor& operator=(const OrderExtractor&) = delete;

  // mojom::OrderExtractor:
  void ExtractOrderInfo(mojom::OrderPatternsInfoPtr mojom_patterns,
                        ExtractOrderInfoCallback callback) override;

  // content::RenderFrameObserver:
  void OnDestruct() override;

 private:
  void BindReceiver(mojo::PendingReceiver<mojom::OrderExtractor> receiver);

  mojo::Receiver<mojom::OrderExtractor> receiver_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_EXTRACTOR_H_
