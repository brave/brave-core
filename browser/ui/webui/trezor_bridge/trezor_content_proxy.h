/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_TREZOR_BRIDGE_TREZOR_CONTENT_PROXY_H_
#define BRAVE_BROWSER_UI_WEBUI_TREZOR_BRIDGE_TREZOR_CONTENT_PROXY_H_

#include <string>

#include "base/macros.h"
#include "brave/components/brave_wallet/browser/trezor_bridge_controller.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_ui_controller.h"

class TrezorBridgeContentObserver;

namespace content {
class BrowserContext;
}

class TrezorContentProxy : public content::WebContentsObserver,
                           public brave_wallet::TrezorBridgeContentProxy {
 public:
  TrezorContentProxy(content::BrowserContext* context);
  ~TrezorContentProxy() override;
  TrezorContentProxy(const TrezorContentProxy&) = delete;
  TrezorContentProxy& operator=(const TrezorContentProxy&) = delete;

  // brave_wallet::TrezorBridgeContentProxy
  void InitWebContents() override;
  void SetObserver(
      brave_wallet::TrezorBridgeContentObserver* observer) override;
  base::WeakPtr<MojoTrezorWebUIController::LibraryController>
  ConnectWithWebUIBridge(
      base::WeakPtr<MojoTrezorWebUIController::Subscriber> subscriber) override;
  bool IsReady() const override;

 protected:
  void RenderProcessGone(base::TerminationStatus status) override;

  void DidFailLoad(content::RenderFrameHost* render_frame_host,
                   const GURL& validated_url,
                   int error_code) override;

  void DocumentOnLoadCompletedInMainFrame(
      content::RenderFrameHost* render_frame_host) override;

 private:
  TrezorBridgeUI* GetWebUIController() const;
  void DestroyContent();
  brave_wallet::TrezorBridgeContentObserver* observer_ = nullptr;
  std::unique_ptr<content::WebContents> web_contents_;
  content::BrowserContext* browser_context_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_BRIDGE_TREZOR_CONTENT_PROXY_H_
