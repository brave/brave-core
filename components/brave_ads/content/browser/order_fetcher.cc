/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/order_fetcher.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/ref_counted.h"
#include "base/timer/timer.h"
#include "brave/components/brave_ads/content/browser/order_patterns_mojom_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace brave_ads {

// One instance per fetch, kept separate from `OrderFetcher` so a superseded
// fetch (see `FetchOrder`) can be resolved without disturbing the new one.
// Ref counted so the reply and timeout paths can each hold it alive
// independently of `OrderFetcher` superseding it.
struct FetchState : public base::RefCounted<FetchState> {
  bool is_complete = false;
  base::OneShotTimer timer;
  OrderFetcher::FetchOrderCallback callback;

 private:
  friend class base::RefCounted<FetchState>;
  ~FetchState() = default;
};

namespace {

void ExtractOrderInfoCallback(
    std::unique_ptr<mojo::Remote<mojom::OrderExtractor>>,
    scoped_refptr<FetchState> fetch_state,
    mojom::OrderInfoPtr mojom_order) {
  if (fetch_state->is_complete) {
    return;
  }
  fetch_state->is_complete = true;
  fetch_state->timer.Stop();
  std::move(fetch_state->callback).Run(std::move(mojom_order));
}

void FetchOrderTimeoutCallback(scoped_refptr<FetchState> fetch_state) {
  if (fetch_state->is_complete) {
    return;
  }
  fetch_state->is_complete = true;
  // No reply means no real answer, not a confirmed "nothing found", so
  // run with a null `OrderInfo` rather than an empty one. `AdsTabHelper`
  // skips notifying `TabManager` on null, leaving any already-cached order
  // value/ID from an earlier page in this tab untouched.
  std::move(fetch_state->callback).Run(nullptr);
}

}  // namespace

OrderFetcher::OrderFetcher(content::WebContents& web_contents)
    : web_contents_(web_contents) {}

OrderFetcher::~OrderFetcher() = default;

void OrderFetcher::FetchOrder(
    mojom::OrderExtractionPatternsInfoPtr mojom_patterns,
    FetchOrderCallback callback) {
  if (active_fetch_state_ && !active_fetch_state_->is_complete) {
    // A page this `OrderFetcher` no longer cares about; resolve its
    // callback now rather than let it race the fetch started below. Not a
    // confirmed "nothing found" either, so null, not empty.
    active_fetch_state_->is_complete = true;
    active_fetch_state_->timer.Stop();
    std::move(active_fetch_state_->callback).Run(nullptr);
  }

  active_fetch_state_ = base::MakeRefCounted<FetchState>();
  active_fetch_state_->callback = std::move(callback);

  content::RenderFrameHost* const primary_render_frame_host =
      web_contents_->GetPrimaryMainFrame();
  if (!primary_render_frame_host->IsRenderFrameLive()) {
    active_fetch_state_->is_complete = true;
    std::move(active_fetch_state_->callback).Run(nullptr);
    return;
  }

  auto order_extractor =
      std::make_unique<mojo::Remote<mojom::OrderExtractor>>();
  primary_render_frame_host->GetRemoteInterfaces()->GetInterface(
      order_extractor->BindNewPipeAndPassReceiver());

  // `order_extractor` must outlive the call; capturing it in the reply
  // keeps it alive exactly until the reply runs, then it is destroyed.
  // A disconnect before any reply is also "no real answer", so this
  // defaults to null too, not an empty `OrderInfo`.
  mojom::OrderExtractor* const raw_order_extractor = order_extractor->get();
  raw_order_extractor->ExtractOrderInfo(
      ToMojomOrderPatterns(*mojom_patterns),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          base::BindOnce(&ExtractOrderInfoCallback, std::move(order_extractor),
                         active_fetch_state_),
          nullptr));

  active_fetch_state_->timer.Start(
      FROM_HERE, kFetchOrderTimeout,
      base::BindOnce(&FetchOrderTimeoutCallback, active_fetch_state_));
}

}  // namespace brave_ads
