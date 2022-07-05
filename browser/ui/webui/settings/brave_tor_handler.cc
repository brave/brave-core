// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/brave_tor_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/browser/tor//tor_profile_service_factory.h"
#include "brave/components/tor/pref_names.h"
#include "brave/components/tor/tor_profile_service.h"
#include "brave/components/tor/tor_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/image_fetcher/image_decoder_impl.h"
#include "components/image_fetcher/core/image_decoder.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"
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
  ~BridgeRequest() override {
    if (captcha_callback_) {
      std::move(captcha_callback_).Run(base::Value());
    }
    if (result_callback_) {
      std::move(result_callback_).Run(base::Value());
    }
  }

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
    if (!value.is_dict() || !value.GetDict().FindString("captcha")) {
      return std::move(captcha_callback_).Run(std::move(value));
    }
    std::string image_data;
    base::Base64Decode(*value.GetDict().FindString("captcha"));
    if (!base::Base64Decode(*value.GetDict().FindString("captcha"),
                            &image_data)) {
      return std::move(captcha_callback_).Run(std::move(value));
    }
    if (!image_decoder_) {
      image_decoder_ = std::make_unique<ImageDecoderImpl>();
    }

    image_decoder_->DecodeImage(image_data, {}, /*data_decoder=*/nullptr,
                                base::BindOnce(&BridgeRequest::OnCaptchaDecoded,
                                               weak_factory_.GetWeakPtr()));
  }

  void OnCaptchaDecoded(const gfx::Image& image) {
    // Re-encode image as PNG and send.
    auto encoded = base::MakeRefCounted<base::RefCountedBytes>();
    if (!gfx::PNGCodec::EncodeBGRASkBitmap(image.AsBitmap(),
                                           /*discard_transparency=*/false,
                                           &encoded->data())) {
      return std::move(captcha_callback_).Run(base::Value());
    }

    base::Value::Dict result;
    result.Set("captcha",
               "data:image/png;base64," + base::Base64Encode(encoded->data()));
    std::move(captcha_callback_).Run(base::Value(std::move(result)));
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
  std::unique_ptr<image_fetcher::ImageDecoder> image_decoder_;

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
  web_ui()->RegisterMessageCallback(
      "brave_tor.setTorEnabled",
      base::BindRepeating(&BraveTorHandler::SetTorEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_tor.isTorEnabled",
      base::BindRepeating(&BraveTorHandler::IsTorEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_tor.isTorManaged",
      base::BindRepeating(&BraveTorHandler::IsTorManaged,
                          base::Unretained(this)));

  local_state_change_registrar_.Init(g_browser_process->local_state());
  local_state_change_registrar_.Add(
      tor::prefs::kTorDisabled,
      base::BindRepeating(&BraveTorHandler::OnTorEnabledChanged,
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
  CHECK_EQ(1u, args.size());

  auto captcha_callback = base::BindOnce(
      &BraveTorHandler::SendResultToJavascript, base::Unretained(this),
      /*reset_request*/ false, args[0].Clone());
  request_ = std::make_unique<BridgeRequest>(
      web_ui()->GetWebContents()->GetBrowserContext(),
      std::move(captcha_callback));
}

void BraveTorHandler::ResolveBridgesCaptcha(const base::Value::List& args) {
  CHECK_EQ(2u, args.size());

  AllowJavascript();

  if (!request_) {
    RejectJavascriptCallback(args[0].Clone(), base::Value());
    return;
  }

  auto bridges_callback = base::BindOnce(
      &BraveTorHandler::SendResultToJavascript, base::Unretained(this),
      /*reset_request*/ true, args[0].Clone());
  request_->ProvideCaptcha(args[1].GetString(), std::move(bridges_callback));
}

void BraveTorHandler::SendResultToJavascript(bool reset_request,
                                             const base::Value& callback_id,
                                             const base::Value& response) {
  AllowJavascript();
  if (response.is_none()) {
    RejectJavascriptCallback(callback_id, response);
  } else {
    ResolveJavascriptCallback(callback_id, response);
  }
  if (reset_request) {
    request_.reset();
  }
}

void BraveTorHandler::SetTorEnabled(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  const bool enabled = args[0].GetBool();
  AllowJavascript();
  TorProfileServiceFactory::SetTorDisabled(!enabled);
}

void BraveTorHandler::IsTorEnabled(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(
      args[0], base::Value(!TorProfileServiceFactory::IsTorDisabled()));
}

void BraveTorHandler::OnTorEnabledChanged() {
  if (IsJavascriptAllowed()) {
    FireWebUIListener("tor-enabled-changed",
                      base::Value(!TorProfileServiceFactory::IsTorDisabled()));
  }
}

void BraveTorHandler::IsTorManaged(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);

  const bool is_managed = g_browser_process->local_state()
                              ->FindPreference(tor::prefs::kTorDisabled)
                              ->IsManaged();
  AllowJavascript();
  ResolveJavascriptCallback(args[0], base::Value(is_managed));
}
