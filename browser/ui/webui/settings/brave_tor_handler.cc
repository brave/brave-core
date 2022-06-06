// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/brave_tor_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/browser/tor//tor_profile_service_factory.h"
#include "brave/components/tor/tor_profile_service.h"
#include "brave/components/tor/tor_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "url/gurl.h"

namespace {

constexpr const char kTorBridgesUrl[] =
    "https://bridges.torproject.org/bridges?transport=obfs4";

constexpr const char16_t kGetCaptchaScript[] =
    uR"js(
  (function() {
    const captchaBox = document.getElementById('captcha-box')
    if (!captchaBox) return null
  
    const captchaImages = captchaBox.getElementsByTagName('img')
    if (!captchaImages || captchaImages.length < 1)
      return null

    const captchaImage = captchaImages[0]
    return { captcha: captchaImage.src }
  })();
)js";

constexpr const char16_t kSendCaptchaScript[] =
    uR"js(
  (function(captcha) {
    const responseField = document.getElementById('captcha_response_field')
    if (!responseField) return null

    responseField.value = captcha

    const submit = document.getElementById('captcha-submit-button')
    if (!submit) return null

    submit.click()
    return { result: true }
  })
)js";

constexpr const char16_t kParseBridgesScript[] =
    uR"js(
  (function() {
    const bridgeLines = document.getElementById('bridgelines')
    if (!bridgeLines) return null
  
    const bridges = bridgeLines.textContent.split('\n').filter(
       (bridge) => { return bridge.trim().length != 0 })
    return {bridges : bridges}
  })();
)js";

constexpr int kIsolatedWorldId = content::ISOLATED_WORLD_ID_CONTENT_END + 1;
}  // namespace

class BridgeRequest : public content::WebContentsObserver {
 public:
  using CaptchaCallback = base::OnceCallback<void(const base::Value& image)>;
  using BridgesCallback = base::OnceCallback<void(const base::Value& bridges)>;

  BridgeRequest(content::BrowserContext* browser_context,
                CaptchaCallback captcha_callback)
      : captcha_callback_(std::move(captcha_callback)) {
    content::WebContents::CreateParams params(browser_context);
    web_contents_ = content::WebContents::Create(params);
    Observe(web_contents_.get());

    const GURL url(kTorBridgesUrl);
    content::NavigationController::LoadURLParams load_params(url);
    web_contents_->GetController().LoadURLWithParams(load_params);
  }

  BridgeRequest(const BridgeRequest&) = delete;
  BridgeRequest& operator=(const BridgeRequest&) = delete;
  ~BridgeRequest() override = default;

  void ProvideCaptcha(const std::string& captcha,
                      BridgesCallback result_callback) {
    DCHECK_EQ(State::kProvideCaptcha, state_);

    result_callback_ = std::move(result_callback);
    const auto script = base::StrCat(
        {kSendCaptchaScript, u"('", base::UTF8ToUTF16(captcha), u"')"});
    render_frame_host_->ExecuteJavaScriptInIsolatedWorld(
        script, base::DoNothing(), kIsolatedWorldId);
    state_ = State::kWaitForBridges;
  }

 private:
  enum class State {
    kLoadCaptcha,
    kProvideCaptcha,
    kWaitForBridges,
  };

  // content::WebContentsObserver:
  void DOMContentLoaded(content::RenderFrameHost* render_frame_host) override {
    render_frame_host_ = render_frame_host;
    switch (state_) {
      case State::kLoadCaptcha:
        render_frame_host_->ExecuteJavaScriptInIsolatedWorld(
            kGetCaptchaScript,
            base::BindOnce(&BridgeRequest::OnGetCaptchaImage,
                           weak_factory_.GetWeakPtr()),
            kIsolatedWorldId);
        state_ = State::kProvideCaptcha;
        break;
      case State::kProvideCaptcha:
        break;
      case State::kWaitForBridges:
        ParseBridges();
        break;
    }
  }

  void WebContentsDestroyed() override {
    Observe(nullptr);

    if (result_callback_) {
      std::move(result_callback_).Run({});
    }
  }

  void OnGetCaptchaImage(base::Value value) {
    std::move(captcha_callback_).Run(std::move(value));
  }

  void ParseBridges() {
    DCHECK_EQ(State::kWaitForBridges, state_);
    render_frame_host_->ExecuteJavaScriptInIsolatedWorld(
        kParseBridgesScript,
        base::BindOnce(&BridgeRequest::OnBridgesParsed,
                       weak_factory_.GetWeakPtr()),
        kIsolatedWorldId);
  }

  void OnBridgesParsed(base::Value value) {
    std::move(result_callback_).Run(std::move(value));
  }

  raw_ptr<content::RenderFrameHost> render_frame_host_ = nullptr;
  CaptchaCallback captcha_callback_;
  BridgesCallback result_callback_;
  State state_ = State::kLoadCaptcha;
  std::unique_ptr<content::WebContents> web_contents_;

  base::WeakPtrFactory<BridgeRequest> weak_factory_{this};
};

BraveTorHandler::BraveTorHandler() = default;
BraveTorHandler::~BraveTorHandler() = default;

void BraveTorHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "brave_tor.getBridgesConfig",
      base::BindRepeating(&BraveTorHandler::GetBridgesConfig,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_tor.setBridgesConfig",
      base::BindRepeating(&BraveTorHandler::SetBridgesConfig,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_tor.requestBridgesCaptcha",
      base::BindRepeating(&BraveTorHandler::RequestBridgesCaptcha,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_tor.resolveBridgesCaptcha",
      base::BindRepeating(&BraveTorHandler::ResolveBridgesCaptcha,
                          base::Unretained(this)));
}

void BraveTorHandler::GetBridgesConfig(const base::Value::List& args) {
  AllowJavascript();

  CHECK_EQ(1u, args.size());
  const auto bridges_config = TorProfileServiceFactory::GetTorBridgesConfig();

  ResolveJavascriptCallback(args[0], bridges_config.ToValue());
}

void BraveTorHandler::SetBridgesConfig(const base::Value::List& args) {
  CHECK_EQ(1u, args.size());
  CHECK(args[0].is_dict());

  const auto bridges_config = tor::BridgesConfig::FromValue(&args[0]);
  CHECK(bridges_config);
  TorProfileServiceFactory::SetTorBridgesConfig(*bridges_config);
}

void BraveTorHandler::RequestBridgesCaptcha(const base::Value::List& args) {
  auto captcha_callback =
      base::BindOnce(&BraveTorHandler::SendResultToJavascript,
                     base::Unretained(this), args[0].Clone());
  request_ = std::make_unique<BridgeRequest>(
      web_ui()->GetWebContents()->GetBrowserContext(),
      std::move(captcha_callback));
}

void BraveTorHandler::ResolveBridgesCaptcha(const base::Value::List& args) {
  AllowJavascript();

  auto bridges_callback =
      base::BindOnce(&BraveTorHandler::SendResultToJavascript,
                     base::Unretained(this), args[0].Clone());
  request_->ProvideCaptcha(args[1].GetString(), std::move(bridges_callback));
}

void BraveTorHandler::SendResultToJavascript(const base::Value& callback_id,
                                             const base::Value& response) {
  AllowJavascript();
  if (response.is_none()) {
    RejectJavascriptCallback(callback_id, response);
  } else {
    ResolveJavascriptCallback(callback_id, response);
  }
}
