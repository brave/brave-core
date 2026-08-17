/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_RENDERER_WEB3_METRICS_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_MISC_METRICS_RENDERER_WEB3_METRICS_RENDER_FRAME_OBSERVER_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/misc_metrics/common/misc_metrics.mojom.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/web/web_navigation_type.h"
#include "url/gurl.h"

namespace content {
class RenderFrame;
}  // namespace content

namespace misc_metrics {

class Web3MetricsRenderFrameObserver : public content::RenderFrameObserver {
 public:
  // `is_web3_enabled_callback` reports whether a wallet provider is being
  // installed into pages. There is nothing to proxy while it returns false, and
  // the proxy is itself observable, so injection waits on it.
  Web3MetricsRenderFrameObserver(
      content::RenderFrame* render_frame,
      base::RepeatingCallback<bool()> is_web3_enabled_callback);
  Web3MetricsRenderFrameObserver(const Web3MetricsRenderFrameObserver&) =
      delete;
  Web3MetricsRenderFrameObserver& operator=(
      const Web3MetricsRenderFrameObserver&) = delete;
  ~Web3MetricsRenderFrameObserver() override;

  // content::RenderFrameObserver:
  void DidStartNavigation(
      const GURL& url,
      std::optional<blink::WebNavigationType> navigation_type) override;
  void DidClearWindowObject() override;

 private:
  // content::RenderFrameObserver:
  void OnDestruct() override;

  bool IsPageValid();
  bool CanInjectProxy();

  // Invoked from the injected main-world script when a proxied web3 method is
  // called.
  void OnWeb3Called();

  mojom::Web3Metrics& GetWeb3Metrics();

  GURL url_;
  base::RepeatingCallback<bool()> is_web3_enabled_callback_;
  std::string install_proxy_script_;
  mojo::Remote<mojom::Web3Metrics> web3_metrics_;
  base::WeakPtrFactory<Web3MetricsRenderFrameObserver> weak_ptr_factory_{this};
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_RENDERER_WEB3_METRICS_RENDER_FRAME_OBSERVER_H_
