/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/trezor_bridge/trezor_bridge_handler.h"

#include <memory>
#include <string>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/logging.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

namespace {

const base::flat_map<std::string, int> trezor_data_resources = {
    {"./data/config.json", IDR_TREZOR_BRIDGE_IFRAME_DATA_CONFIG},
    {"./data/coins.json", IDR_TREZOR_BRIDGE_IFRAME_DATA_COINS},
    {"./data/messages/messages.json",
     IDR_TREZOR_BRIDGE_IFRAME_DATA_MESSAGES_MESSAGES},
    {"./data/messages/messages-v6.json",
     IDR_TREZOR_BRIDGE_IFRAME_DATA_MESSAGES_MESSAGES_V6},
    {"./data/messages/messages-v7.json",
     IDR_TREZOR_BRIDGE_IFRAME_DATA_MESSAGES_MESSAGES_V7},
    {"./data/messages/messages-v8.json",
     IDR_TREZOR_BRIDGE_IFRAME_DATA_MESSAGES_MESSAGES_V8},
    {"./data/firmware/1/releases.json",
     IDR_TREZOR_BRIDGE_IFRAME_DATA_FIRMWARE_RELEASES1},
    {"./data/firmware/2/releases.json",
     IDR_TREZOR_BRIDGE_IFRAME_DATA_FIRMWARE_RELEASES2},
    {"./data/bridge/releases.json",
     IDR_TREZOR_BRIDGE_IFRAME_DATA_BRIDGE_RELEASES},
    {"./data/bridge/latest.txt",
     IDR_TREZOR_BRIDGE_IFRAME_DATA_BRIDGE_LATEST},
};

// TrezorSuite local address.
const char kTrezorSuiteURL[] = "http://127.0.0.1:21325";

net::NetworkTrafficAnnotationTag GetTrezorBridgeNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("trezor_bridge", R"(
          semantics {
            sender: "Trezor Bridge"
            description:
              "This bridge is used to communicate with TrezorSuite app "
            trigger:
              "Triggered by actions in brave://wallet."
            data:
              "Options of the commands."
            destination: WEBSITE
          }
        )");
}

std::unique_ptr<network::SimpleURLLoader> CreateURLLoader(
    const GURL& gurl,
    const std::string& method,
    const std::string& body) {
  std::unique_ptr<network::ResourceRequest> request =
      std::make_unique<network::ResourceRequest>();
  request->url = gurl;
  request->method = method;
  // Use trezor.io origin because Trezor Sute accepts connections only from
  // trezor.io domains.
  request->headers.SetHeader(net::HttpRequestHeaders::kOrigin,
                             "https://brave.trezor.io");
  request->headers.SetHeader(net::HttpRequestHeaders::kContentType,
                             "text/plain");

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetTrezorBridgeNetworkTrafficAnnotationTag());
  if (!body.empty())
    url_loader->AttachStringForUpload(body, "text/plain");
  return url_loader;
}

}  // namespace

TrezorBridgeHandler::TrezorBridgeHandler(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory) {}

TrezorBridgeHandler::~TrezorBridgeHandler() {}

void TrezorBridgeHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "trezor-fetch",
      base::BindRepeating(&TrezorBridgeHandler::HandleFetchRequest,
                          base::Unretained(this)));
}

void TrezorBridgeHandler::HandleFetchRequest(const base::ListValue* args) {
  std::string url;
  CHECK(args->GetString(1, &url));
  auto path = url.substr(0, url.find("?"));
  DLOG(INFO) << "path:" << path;
  if (trezor_data_resources.contains(path)) {
    auto resource_id = trezor_data_resources.at(path);
    const ui::ResourceBundle& resource_bundle =
        ui::ResourceBundle::GetSharedInstance();
    const std::string& resource_text =
        resource_bundle.LoadDataResourceString(resource_id);
    RespondRequestCallback(args->GetList()[0], true, resource_text, "ok");
    return;
  }
  const auto trezor_suite_url = GURL(kTrezorSuiteURL);
  const auto requested_url = GURL(url);
  bool trezor_suite = (requested_url.host() == trezor_suite_url.host()) &&
                      (requested_url.port() == trezor_suite_url.port()) &&
                      (requested_url.scheme() == trezor_suite_url.scheme());
  if (!trezor_suite) {
    RespondRequestCallback(
        args->GetList()[0], false, std::string(),
        l10n_util::GetStringUTF8(IDS_TREZOR_UNKNOWN_REQUEST));
    return;
  }
  const base::Value& options = args->GetList()[2];
  std::string method = "GET";
  const std::string* method_value = options.FindStringKey("method");
  if (method_value)
    method = *method_value;
  std::string body;
  const std::string* body_value = options.FindStringKey("body");
  if (body_value)
    body = *body_value;

  auto url_loader = CreateURLLoader(requested_url, method, body);
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&TrezorBridgeHandler::OnRequestResponse,
                     weak_ptr_factory_.GetWeakPtr(), iter,
                     args->GetList()[0].Clone()));
}

void TrezorBridgeHandler::RespondRequestCallback(
    const base::Value& callback_id,
    bool success,
    const std::string& text,
    const std::string& status_text) {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetBoolKey("ok", success);
  dict.SetStringKey("text", text);
  dict.SetStringKey("statusText", status_text);
  if (callback_for_testing_) {
    std::move(callback_for_testing_).Run(dict);
    return;
  }
  AllowJavascript();
  ResolveJavascriptCallback(callback_id, dict);
}

void TrezorBridgeHandler::OnRequestResponse(
    SimpleURLLoaderList::iterator iter,
    base::Value callback_id,
    std::unique_ptr<std::string> response_body) {
  auto* url_loader = iter->get();
  int error_code = url_loader->NetError();
  int response_code = -1;
  if (url_loader->ResponseInfo() && url_loader->ResponseInfo()->headers)
    response_code = url_loader->ResponseInfo()->headers->response_code();
  url_loaders_.erase(iter);
  bool success = (error_code == net::OK && response_code == net::HTTP_OK);
  if (!success) {
    VLOG(1) << "Fail to get connected peers, error_code = " << error_code
            << " response_code = " << response_code;
  }
  std::string response_text;
  if (success && response_body) {
    response_text = *response_body;
  }
  RespondRequestCallback(callback_id, success, response_text,
                         success ? "ok" : "error");
}
