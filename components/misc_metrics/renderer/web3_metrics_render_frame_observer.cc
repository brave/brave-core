/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/renderer/web3_metrics_render_frame_observer.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "brave/components/safe_builtins/renderer/safe_builtins_helpers.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/renderer/render_frame.h"
#include "gin/function_template.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/origin.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-microtask-queue.h"

namespace misc_metrics {

Web3MetricsRenderFrameObserver::Web3MetricsRenderFrameObserver(
    content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame),
      install_proxy_script_(base::StrCat(
          {"(",
           ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
               IDR_MISC_METRICS_WEB3_INSTALL_PROXY_JS),
           ")"})) {}

Web3MetricsRenderFrameObserver::~Web3MetricsRenderFrameObserver() = default;

void Web3MetricsRenderFrameObserver::DidStartNavigation(
    const GURL& url,
    std::optional<blink::WebNavigationType> navigation_type) {
  url_ = url;
}

bool Web3MetricsRenderFrameObserver::IsPageValid() {
  // There could be empty, invalid and "about:blank" URLs,
  // they should fallback to the main frame rules.
  if (url_.is_empty() || !url_.is_valid() || url_.spec() == "about:blank") {
    url_ = url::Origin(render_frame()->GetWebFrame()->GetSecurityOrigin())
               .GetURL();
  }
  return url_.SchemeIsHTTPOrHTTPS();
}

bool Web3MetricsRenderFrameObserver::CanInjectProxy() {
  if (!IsPageValid()) {
    return false;
  }

  if (!render_frame()->GetWebFrame()->GetDocument().IsSecureContext()) {
    return false;
  }

  // Scripts can't be executed on provisional frames.
  if (render_frame()->GetWebFrame()->IsProvisional()) {
    return false;
  }

  return true;
}

void Web3MetricsRenderFrameObserver::DidClearWindowObject() {
  if (!CanInjectProxy()) {
    return;
  }

  CHECK(render_frame());
  auto* web_frame = render_frame()->GetWebFrame();
  v8::Isolate* isolate = web_frame->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = web_frame->MainWorldScriptContext();
  if (context.IsEmpty()) {
    return;
  }
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  // Load the installer function in a safe-builtins closure.
  v8::Local<v8::Value> installer_value;
  if (!brave::LoadScriptWithSafeBuiltins(web_frame, install_proxy_script_)
           .ToLocal(&installer_value) ||
      !installer_value->IsFunction()) {
    return;
  }

  // Native callback that JS will invoke when a proxied method is called.
  v8::Local<v8::Function> callback =
      gin::CreateFunctionTemplate(
          isolate,
          base::BindRepeating(&Web3MetricsRenderFrameObserver::OnWeb3Called,
                              weak_ptr_factory_.GetWeakPtr()))
          ->GetFunction(context)
          .ToLocalChecked();

  v8::Local<v8::Value> args[] = {callback};
  std::ignore = web_frame->CallFunctionEvenIfScriptDisabled(
      installer_value.As<v8::Function>(), context->Global(), std::size(args),
      args);
}

void Web3MetricsRenderFrameObserver::OnWeb3Called() {
  GetWeb3Metrics().RecordDappVisit();
}

mojom::Web3Metrics& Web3MetricsRenderFrameObserver::GetWeb3Metrics() {
  CHECK(render_frame());
  if (!web3_metrics_.is_bound()) {
    render_frame()->GetBrowserInterfaceBroker().GetInterface(
        web3_metrics_.BindNewPipeAndPassReceiver());
  }
  return *web3_metrics_.get();
}

void Web3MetricsRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace misc_metrics
