// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/script_injector/renderer/script_injector_render_frame_observer.h"

#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace script_injector {

ScriptInjectorRenderFrameObserver::ScriptInjectorRenderFrameObserver(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame), weak_ptr_factory_(this) {
  render_frame->GetAssociatedInterfaceRegistry()
      ->AddInterface<script_injector::mojom::ScriptInjector>(
          base::BindRepeating(
              &ScriptInjectorRenderFrameObserver::BindToReceiver,
              weak_ptr_factory_.GetWeakPtr()));
}

void ScriptInjectorRenderFrameObserver::OnDestruct() {
  delete this;
}

void ScriptInjectorRenderFrameObserver::BindToReceiver(
    mojo::PendingAssociatedReceiver<script_injector::mojom::ScriptInjector>
        pending_receiver) {
  receivers_.Add(this, std::move(pending_receiver));
}

ScriptInjectorRenderFrameObserver::~ScriptInjectorRenderFrameObserver() =
    default;

// static
blink::mojom::WantResultOption
ScriptInjectorRenderFrameObserver::CheckIfWantResult(
    const RequestAsyncExecuteScriptCallback& callback) {
  return callback.is_null() ? blink::mojom::WantResultOption::kNoResult
                            : blink::mojom::WantResultOption::kWantResult;
}
void ScriptInjectorRenderFrameObserver::RequestAsyncExecuteScript(
    int32_t world_id,
    const std::u16string& script,
    blink::mojom::UserActivationOption user_activation,
    blink::mojom::PromiseResultOption await_promise,
    RequestAsyncExecuteScriptCallback callback) {
  blink::WebScriptSource web_script_source =
      blink::WebScriptSource(blink::WebString::FromUTF16(script));

  auto want_result = CheckIfWantResult(callback);

  render_frame()->GetWebFrame()->RequestExecuteScript(
      world_id, base::span_from_ref(web_script_source), user_activation,
      blink::mojom::EvaluationTiming::kAsynchronous,
      blink::mojom::LoadEventBlockingOption::kDoNotBlock,
      base::BindOnce(
          [](RequestAsyncExecuteScriptCallback callback,
             blink::mojom::WantResultOption want_result,
             std::optional<base::Value> value, base::TimeTicks start_time) {
            if (want_result == blink::mojom::WantResultOption::kWantResult) {
              std::move(callback).Run(value ? std::move(*value)
                                            : base::Value());
            }
          },
          std::move(callback), want_result),
      blink::BackForwardCacheAware::kAllow, want_result, await_promise);
}

}  // namespace script_injector
