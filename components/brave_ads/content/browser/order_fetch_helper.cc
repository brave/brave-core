/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/order_fetch_helper.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace brave_ads {

OrderFetchHelper::OrderFetchHelper() = default;

OrderFetchHelper::~OrderFetchHelper() = default;

void OrderFetchHelper::FetchOrder(
    content::RenderFrameHost& primary_render_frame_host,
    mojom::OrderPatternsInfoPtr mojom_order_patterns,
    base::TimeDelta timeout,
    base::OnceCallback<void(mojom::ExtractedOrderInfoPtr)> callback) {
  callback_ = std::move(callback);

  if (!primary_render_frame_host.IsRenderFrameLive()) {
    // Posted rather than run synchronously so that `callback_` is always
    // invoked asynchronously, matching the mojo reply and timeout paths.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback_), mojom::ExtractedOrderInfo::New()));
    return;
  }

  primary_render_frame_host.GetRemoteInterfaces()->GetInterface(
      order_extractor_remote_.BindNewPipeAndPassReceiver());

  order_extractor_remote_->ExtractOrder(
      std::move(mojom_order_patterns),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          base::BindOnce(&OrderFetchHelper::ExtractOrderCallback,
                         weak_factory_.GetWeakPtr()),
          mojom::ExtractedOrderInfo::New()));

  timer_.Start(FROM_HERE, timeout,
               base::BindOnce(&OrderFetchHelper::TimeoutCallback,
                              base::Unretained(this)));
}

void OrderFetchHelper::ExtractOrderCallback(
    mojom::ExtractedOrderInfoPtr mojom_extracted_order) {
  CHECK(mojom_extracted_order);

  if (!callback_) {
    // Callback has already been invoked.
    return;
  }

  timer_.Stop();
  std::move(callback_).Run(std::move(mojom_extracted_order));
}

void OrderFetchHelper::TimeoutCallback() {
  if (!callback_) {
    // Callback has already been invoked.
    return;
  }

  std::move(callback_).Run(mojom::ExtractedOrderInfo::New());
}

}  // namespace brave_ads
