/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/trezor_bridge/trezor_content_proxy.h"

#include <string>

#include "base/macros.h"
#include "brave/browser/ui/webui/trezor_bridge/trezor_bridge_ui.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/trezor_bridge/mojo_trezor_web_ui_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "net/base/load_states.h"

namespace {

content::WebContents::CreateParams GetWebContentsCreateParams(
    content::BrowserContext* browser_context) {
  content::WebContents::CreateParams create_params(browser_context);
  create_params.initially_hidden = true;
  return create_params;
}

}  // namespace

TrezorContentProxy::TrezorContentProxy(content::BrowserContext* context)
    : TrezorBridgeContentProxy(context), browser_context_(context) {}

TrezorContentProxy::~TrezorContentProxy() {}

void TrezorContentProxy::InitWebContents() {
  if (!web_contents_) {
    web_contents_ = content::WebContents::Create(
        GetWebContentsCreateParams(browser_context_));
    content::WebContentsObserver::Observe(web_contents_.get());
  }

  web_contents_->GetController().LoadURL(
      GURL(kBraveTrezorBridgeURL), content::Referrer(),
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL, std::string());
}

void TrezorContentProxy::DestroyContent() {
  content::WebContentsObserver::Observe(nullptr);
  web_contents_.reset();
}

void TrezorContentProxy::RenderProcessGone(base::TerminationStatus status) {
  DestroyContent();
  if (observer_)
    observer_->BridgeFail();
}

void TrezorContentProxy::DidFailLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url,
    int error_code) {
  DestroyContent();
  if (observer_)
    observer_->BridgeFail();
}

void TrezorContentProxy::DocumentOnLoadCompletedInMainFrame(
    content::RenderFrameHost* render_frame_host) {
  DCHECK_EQ(web_contents_->GetVisibleURL().host(), kBraveTrezorBridgeHost);
  if (observer_)
    observer_->BridgeReady();
}

TrezorBridgeUI* TrezorContentProxy::GetWebUIController() const {
  content::WebUI* const webui = web_contents_->GetWebUI();
  if (!webui || !webui->GetController()) {
    return nullptr;
  }

  TrezorBridgeUI* webui_controller =
      webui->GetController()->template GetAs<TrezorBridgeUI>();
  return webui_controller;
}

bool TrezorContentProxy::IsReady() const {
  if (!web_contents_)
    return false;
  if ((web_contents_->GetVisibleURL().host() != kBraveTrezorBridgeHost) &&
      (web_contents_->GetLoadState().state != net::LOAD_STATE_IDLE)) {
    return false;
  }
  return GetWebUIController() != nullptr;
}

base::WeakPtr<MojoTrezorWebUIController::LibraryController>
TrezorContentProxy::ConnectWithWebUIBridge(
    base::WeakPtr<MojoTrezorWebUIController::Subscriber> subscriber) {
  if (!IsReady())
    return nullptr;
  TrezorBridgeUI* webui_controller = GetWebUIController();
  webui_controller->SetSubscriber(std::move(subscriber));
  return webui_controller->controller();
}

void TrezorContentProxy::SetObserver(
    brave_wallet::TrezorBridgeContentObserver* observer) {
  observer_ = observer;
}
