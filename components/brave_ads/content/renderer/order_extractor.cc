/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/renderer/order_extractor.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/content/renderer/order_extractor_selector_util.h"
#include "brave/components/brave_ads/content/renderer/order_id_extractor_util.h"
#include "brave/components/brave_ads/content/renderer/order_value_extractor_util.h"
#include "brave/components/brave_ads/content/renderer/order_value_match_info.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_node.h"

namespace brave_ads {

OrderExtractor::OrderExtractor(content::RenderFrame* render_frame,
                               service_manager::BinderRegistry* registry)
    : content::RenderFrameObserver(render_frame) {
  if (!render_frame->IsMainFrame()) {
    return;
  }
  registry->AddInterface<mojom::OrderExtractor>(base::BindRepeating(
      &OrderExtractor::BindReceiver, base::Unretained(this)));
}

OrderExtractor::~OrderExtractor() = default;

void OrderExtractor::ExtractOrder(
    mojom::OrderPatternsInfoPtr mojom_order_patterns,
    ExtractOrderCallback callback) {
  CHECK(mojom_order_patterns);

  blink::WebDocument web_document =
      render_frame()->GetWebFrame()->GetDocument();
  const std::vector<blink::WebNode> text_nodes = CollectTextNodes(web_document);

  mojom::ExtractedOrderInfoPtr mojom_extracted_order =
      mojom::ExtractedOrderInfo::New();
  mojom_extracted_order->id =
      MaybeExtractOrderId(web_document, text_nodes, *mojom_order_patterns);
  if (std::optional<OrderValueMatchInfo> value_match = MaybeExtractOrderValue(
          web_document, text_nodes, *mojom_order_patterns)) {
    mojom_extracted_order->value = std::move(value_match->value);
    mojom_extracted_order->currency = std::move(value_match->currency);
  }

  std::move(callback).Run(std::move(mojom_extracted_order));
}

void OrderExtractor::OnDestruct() {
  delete this;
}

void OrderExtractor::BindReceiver(
    mojo::PendingReceiver<mojom::OrderExtractor> receiver) {
  order_extractor_receiver_.reset();
  order_extractor_receiver_.Bind(std::move(receiver));
}

}  // namespace brave_ads
