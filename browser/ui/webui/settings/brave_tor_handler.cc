// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/brave_tor_handler.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_writer.h"
#include "base/memory/ref_counted_memory.h"
#include "base/values.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/tor/pref_names.h"
#include "brave/components/tor/tor_profile_service.h"
#include "brave/components/tor/tor_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/image_fetcher/image_decoder_impl.h"
#include "components/image_fetcher/core/image_decoder.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "net/url_request/url_request.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

namespace {

// https://gitlab.torproject.org/tpo/anti-censorship/rdsys/-/blob/main/doc/moat.md

constexpr char kTorBridgesFetchUrl[] =
    "https://bridges.torproject.org/moat/fetch";
constexpr char kTorBridgesCheckUrl[] =
    "https://bridges.torproject.org/moat/check";

constexpr char kMoatVersion[] = "0.1.0";

constexpr char kMoatShimToken[] = "LVOippNS8UiKLH6kXf1D8pI1clLc";

constexpr net::NetworkTrafficAnnotationTag kTorBridgesMoatAnnotation =
    net::DefineNetworkTrafficAnnotation("brave_tor_bridges", R"(
    semantics {
      sender:
        "Brave Tor Handler"
      description:
        "This service sends requests to the Tor bridges server."
      trigger:
        "When user requests bridges from settings."
      destination: WEBSITE
    }
    policy {
      cookies_allowed: NO
    })");

constexpr size_t kMaxBodySize = 256 * 1024;

base::Value FetchCaptchaData() {
  base::Value::Dict data;
  data.Set("type", "client-transports");
  data.Set("version", kMoatVersion);
  base::Value::List supported_transports;

  supported_transports.Append("obfs4");
  data.Set("supported", std::move(supported_transports));

  base::Value::List list;
  list.Append(std::move(data));

  base::Value::Dict result;
  result.Set("data", std::move(list));
  return base::Value(std::move(result));
}

base::Value SolveCaptchaData(const std::string& challenge,
                             const std::string& solution) {
  base::Value::Dict data;
  data.Set("id", "2");
  data.Set("version", kMoatVersion);
  data.Set("qrcode", "false");
  data.Set("type", "moat-solution");
  data.Set("transport", "obfs4");
  data.Set("challenge", challenge);
  data.Set("solution", solution);

  base::Value::List list;
  list.Append(std::move(data));

  base::Value::Dict result;
  result.Set("data", std::move(list));
  return base::Value(std::move(result));
}

}  // namespace

// Requests TOR bridges from moat API.
class BridgeRequest {
 public:
  using CaptchaCallback = base::OnceCallback<void(const base::Value& image)>;
  using BridgesCallback = base::OnceCallback<void(const base::Value& bridges)>;

  BridgeRequest(content::BrowserContext* browser_context,
                CaptchaCallback captcha_callback)
      : url_loader_factory_(browser_context->GetDefaultStoragePartition()
                                ->GetURLLoaderFactoryForBrowserProcess()),
        captcha_callback_(std::move(captcha_callback)) {
    // Fetch the CAPTCHA challenge.
    simple_url_loader_ =
        MakeMoatRequest(GURL(kTorBridgesFetchUrl), FetchCaptchaData(),
                        base::BindOnce(&BridgeRequest::OnCaptchaResponse,
                                       weak_factory_.GetWeakPtr()));
  }

  BridgeRequest(const BridgeRequest&) = delete;
  BridgeRequest& operator=(const BridgeRequest&) = delete;
  ~BridgeRequest() {
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

    // Check the solution of the challenge.
    simple_url_loader_ =
        MakeMoatRequest(GURL(kTorBridgesCheckUrl),
                        SolveCaptchaData(captcha_challenge_, captcha),
                        base::BindOnce(&BridgeRequest::OnBridgesResponse,
                                       weak_factory_.GetWeakPtr()));

    state_ = State::kWaitForBridges;
  }

 private:
  enum class State {
    kLoadCaptcha,
    kProvideCaptcha,
    kWaitForBridges,
  };

  void OnCaptchaResponse(std::optional<std::string> response_body) {
    simple_url_loader_.reset();

    if (!response_body) {
      OnCaptchaParsed(base::unexpected("Request has failed."));
    } else {
      data_decoder::DataDecoder::ParseJsonIsolated(
          *response_body, base::BindOnce(&BridgeRequest::OnCaptchaParsed,
                                         weak_factory_.GetWeakPtr()));
    }
  }

  void OnCaptchaParsed(data_decoder::DataDecoder::ValueOrError value) {
    if (!value.has_value() || !value->is_dict()) {
      return std::move(captcha_callback_).Run(base::Value());
    }
    const auto* data = value->GetDict().FindList("data");
    if (!data || data->empty()) {
      return std::move(captcha_callback_).Run(base::Value());
    }
    const auto& captcha = data->front();
    if (!captcha.is_dict() || !captcha.GetDict().FindString("image") ||
        !captcha.GetDict().FindString("challenge")) {
      return std::move(captcha_callback_).Run(base::Value());
    }

    std::string image_data;
    if (!base::Base64Decode(*captcha.GetDict().FindString("image"),
                            &image_data)) {
      return std::move(captcha_callback_).Run(base::Value());
    }

    captcha_challenge_ = *captcha.GetDict().FindString("challenge");

    if (!image_decoder_) {
      image_decoder_ = std::make_unique<ImageDecoderImpl>();
    }

    image_decoder_->DecodeImage(image_data, {}, /*data_decoder=*/nullptr,
                                base::BindOnce(&BridgeRequest::OnCaptchaDecoded,
                                               weak_factory_.GetWeakPtr()));
  }

  void OnCaptchaDecoded(const gfx::Image& image) {
    // Re-encode image as PNG and send.
    std::vector<unsigned char> encoded;
    if (!gfx::PNGCodec::EncodeBGRASkBitmap(image.AsBitmap(),
                                           /*discard_transparency=*/false,
                                           &encoded)) {
      return std::move(captcha_callback_).Run(base::Value());
    }

    base::Value::Dict result;
    result.Set("captcha",
               "data:image/png;base64," + base::Base64Encode(encoded));
    std::move(captcha_callback_).Run(base::Value(std::move(result)));
    state_ = State::kProvideCaptcha;
  }

  void OnBridgesResponse(std::optional<std::string> response_body) {
    simple_url_loader_.reset();

    if (!response_body) {
      OnBridgesParsed(base::unexpected("Request has failed."));
    } else {
      data_decoder::DataDecoder::ParseJsonIsolated(
          *response_body, base::BindOnce(&BridgeRequest::OnBridgesParsed,
                                         weak_factory_.GetWeakPtr()));
    }
  }

  void OnBridgesParsed(data_decoder::DataDecoder::ValueOrError value) {
    if (!value.has_value() || !value->is_dict()) {
      return std::move(result_callback_).Run(base::Value());
    }
    const auto* data = value->GetDict().FindList("data");
    if (!data || data->empty() || !data->front().is_dict() ||
        !data->front().GetDict().FindList("bridges")) {
      return std::move(result_callback_).Run(base::Value());
    }

    std::move(result_callback_).Run(data->front());
  }

  std::unique_ptr<network::SimpleURLLoader> MakeMoatRequest(
      const GURL& url,
      const base::Value& data,
      network::SimpleURLLoader::BodyAsStringCallback response_callback) {
    auto request = std::make_unique<network::ResourceRequest>();
    request->url = url;
    request->method = net::HttpRequestHeaders::kPostMethod;
    request->credentials_mode = network::mojom::CredentialsMode::kOmit;
    request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES |
                          net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;
    request->headers.SetHeader("Content-Type", "application/vnd.api+json");
    request->headers.SetHeader("shim-token", kMoatShimToken);

    auto url_loader = network::SimpleURLLoader::Create(
        std::move(request), kTorBridgesMoatAnnotation);
    if (!data.is_none()) {
      std::string data_content;
      base::JSONWriter::Write(data, &data_content);
      url_loader->AttachStringForUpload(data_content);
    }
    url_loader->DownloadToString(url_loader_factory_.get(),
                                 std::move(response_callback), kMaxBodySize);

    return url_loader;
  }

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  CaptchaCallback captcha_callback_;
  BridgesCallback result_callback_;
  State state_ = State::kLoadCaptcha;

  std::string captcha_challenge_;
  std::unique_ptr<image_fetcher::ImageDecoder> image_decoder_;

  std::unique_ptr<network::SimpleURLLoader> simple_url_loader_;

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

  ResolveJavascriptCallback(args[0], bridges_config.ToValue(false));
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
    RejectJavascriptCallback(args[0], base::Value());
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
      args[0], base::Value(!TorProfileServiceFactory::IsTorDisabled(
                   web_ui()->GetWebContents()->GetBrowserContext())));
}

void BraveTorHandler::OnTorEnabledChanged() {
  if (IsJavascriptAllowed()) {
    FireWebUIListener("tor-enabled-changed",
                      base::Value(!TorProfileServiceFactory::IsTorDisabled(
                          web_ui()->GetWebContents()->GetBrowserContext())));
  }
}

void BraveTorHandler::IsTorManaged(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);

  const bool is_managed = TorProfileServiceFactory::IsTorManaged(
      web_ui()->GetWebContents()->GetBrowserContext());

  AllowJavascript();
  ResolveJavascriptCallback(args[0], base::Value(is_managed));
}
