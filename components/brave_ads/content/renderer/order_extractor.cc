/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/renderer/order_extractor.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/content/renderer/order_extractor_dom_util.h"
#include "brave/components/brave_ads/content/renderer/order_id_extractor_util.h"
#include "brave/components/brave_ads/content/renderer/order_value_extractor_util.h"
#include "brave/components/brave_ads/content/renderer/order_value_match_info.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_node.h"
#include "third_party/re2/src/re2/re2.h"

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

void OrderExtractor::ExtractOrderInfo(
    mojom::OrderPatternsInfoPtr mojom_patterns,
    ExtractOrderInfoCallback callback) {
  blink::WebDocument web_document =
      render_frame()->GetWebFrame()->GetDocument();
  mojom::OrderInfoPtr mojom_order = mojom::OrderInfo::New();

  const std::vector<blink::WebNode> text_nodes = CollectTextNodes(web_document);

  if (std::optional<std::string> selected =
          ResolveSelector(web_document, mojom_patterns->id_selectors)) {
    mojom_order->id = std::move(selected);
  } else {
    mojom_order->id =
        FindCapturedId(web_document, text_nodes, mojom_patterns->id_regexes);
  }

  // A configured selector already pins the value-bearing element directly,
  // so unlike the generic heuristic below it does not need the currency
  // regex to disambiguate it from surrounding page text; some merchants
  // render the bare total with no currency symbol in that same element
  // (e.g. a nearby column header carries the currency instead). Gating on
  // `ExtractBareNumber` succeeding, rather than accepting the selector
  // match outright, still lets a stale selector (matching non-numeric
  // content) fall through to the generic heuristic below. `currency` is
  // left unset rather than guessed when no configured currency regex
  // matches the selector-resolved text.
  if (std::optional<std::string> selected =
          ResolveSelector(web_document, mojom_patterns->value_selectors)) {
    mojom_order->value = ExtractBareNumber(*selected);
    if (mojom_order->value) {
      mojom_order->currency =
          MatchCurrency(*selected, mojom_patterns->value_currency_regexes);
    }
  }

  if (!mojom_order->value) {
    const re2::RE2 keyword_regex =
        JoinRegexes(mojom_patterns->value_keyword_regexes);
    if (keyword_regex.ok()) {
      if (std::optional<OrderValueMatchInfo> match = FindLabeledTextNodeValue(
              text_nodes, mojom_patterns->value_currency_regexes, keyword_regex,
              mojom_patterns->number_of_ancestor_levels_to_search)) {
        mojom_order->value = ExtractBareNumber(match->value);
        if (mojom_order->value) {
          mojom_order->currency = match->currency;
        }
      }
    }
  }

  std::move(callback).Run(std::move(mojom_order));
}

void OrderExtractor::OnDestruct() {
  delete this;
}

void OrderExtractor::BindReceiver(
    mojo::PendingReceiver<mojom::OrderExtractor> receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

}  // namespace brave_ads
