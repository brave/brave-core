/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_RENDERER_BRAVE_CONTENT_RENDERER_CLIENT_H_
#define BRAVE_RENDERER_BRAVE_CONTENT_RENDERER_CLIENT_H_

#include <memory>

#include "brave/components/brave_search/renderer/brave_search_service_worker_holder.h"
#include "chrome/renderer/chrome_content_renderer_client.h"
#include "v8/include/v8.h"

class BraveRenderThreadObserver;
class GURL;

namespace blink {
class WebServiceWorkerContextProxy;
}

class BraveContentRendererClient : public ChromeContentRendererClient {
 public:
  BraveContentRendererClient();
  BraveContentRendererClient(const BraveContentRendererClient&) = delete;
  BraveContentRendererClient& operator=(const BraveContentRendererClient&) =
      delete;
  ~BraveContentRendererClient() override;

  void RenderThreadStarted() override;
  void SetRuntimeFeaturesDefaultsBeforeBlinkInitialization() override;
  void RenderFrameCreated(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  void WillEvaluateServiceWorkerOnWorkerThread(
      blink::WebServiceWorkerContextProxy* context_proxy,
      v8::Local<v8::Context> v8_context,
      int64_t service_worker_version_id,
      const GURL& service_worker_scope,
      const GURL& script_url) override;
  void WillDestroyServiceWorkerContextOnWorkerThread(
      v8::Local<v8::Context> v8_context,
      int64_t service_worker_version_id,
      const GURL& service_worker_scope,
      const GURL& script_url) override;

 private:
  std::unique_ptr<BraveRenderThreadObserver> brave_observer_;
  brave_search::BraveSearchServiceWorkerHolder
      brave_search_service_worker_holder_;
};

#endif  // BRAVE_RENDERER_BRAVE_CONTENT_RENDERER_CLIENT_H_
