/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TREZOR_BRIDGE_BROWSER_TREZOR_CONTENT_PROXY_H_
#define BRAVE_COMPONENTS_TREZOR_BRIDGE_BROWSER_TREZOR_CONTENT_PROXY_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "brave/components/trezor_bridge/browser/mojo_trezor_web_ui_controller.h"
#include "content/public/browser/web_contents_observer.h"

class TrezorBridgeContentObserver;

namespace content {
class BrowserContext;
}

class TrezorContentProxy : public content::WebContentsObserver {
 public:
  explicit TrezorContentProxy(content::BrowserContext* context);
  ~TrezorContentProxy() override;
  TrezorContentProxy(const TrezorContentProxy&) = delete;
  TrezorContentProxy& operator=(const TrezorContentProxy&) = delete;

  void InitWebContents();
  void SetObserver(TrezorBridgeContentObserver* observer);
  base::WeakPtr<MojoTrezorWebUIController::LibraryController>
  ConnectWithWebUIBridge(
      base::WeakPtr<MojoTrezorWebUIController::Subscriber> subscriber);
  bool IsReady() const;

 protected:
  // content::WebContentsObserver
  void RenderProcessGone(base::TerminationStatus status) override;

  void DidFailLoad(content::RenderFrameHost* render_frame_host,
                   const GURL& validated_url,
                   int error_code) override;

  void DocumentOnLoadCompletedInMainFrame(
      content::RenderFrameHost* render_frame_host) override;

 private:
  MojoTrezorWebUIController* GetWebUIController() const;
  void DestroyContent();

  TrezorBridgeContentObserver* observer_ = nullptr;
  std::unique_ptr<content::WebContents> web_contents_;
  content::BrowserContext* browser_context_ = nullptr;
};

#endif  // BRAVE_COMPONENTS_TREZOR_BRIDGE_BROWSER_TREZOR_CONTENT_PROXY_H_
