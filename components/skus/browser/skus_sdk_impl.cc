// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/browser/skus_sdk_impl.h"

#include <vector>

#include "base/environment.h"
#include "base/json/json_writer.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/skus/browser/br-rs/brave-rewards-cxx/src/wrapper.hpp"
#include "brave/components/skus/browser/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "net/base/load_flags.h"
#include "net/base/privacy_mode.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"
#include "url/gurl.h"

using namespace std;
using namespace rust::cxxbridge1;
using namespace brave_rewards;

namespace {

// TODO: fix me. I set a completely arbitrary size!
const int kMaxResponseSize = 1000000;  // 1Mb

SkusSdkImpl* g_SkusSdk = NULL;

// START: hack code - remove me
// rust::String's std::string operator gave linker errors :(
// .c_str() not usable because function itself isn't const
// .data is not terminating string
std::string ruststring_2_stdstring(rust::String in) {
  std::string out = "";
  rust::String::iterator it = in.begin();
  while (it != in.end()) {
    out += (char)*it;
    it++;
  }
  return out;
}
std::string ruststr_2_stdstring(rust::cxxbridge1::Str in) {
  std::string out = "";
  rust::cxxbridge1::Str::iterator it = in.begin();
  while (it != in.end()) {
    out += (char)*it;
    it++;
  }
  return out;
}
// END: hack code - remove me

class SkusSdkFetcher {
 public:
  explicit SkusSdkFetcher(scoped_refptr<network::SharedURLLoaderFactory>);
  ~SkusSdkFetcher();

  void BeginFetch(
      const brave_rewards::HttpRequest& req,
      rust::cxxbridge1::Fn<
          void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>,
               brave_rewards::HttpResponse)> callback,
      rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> ctx);

 private:
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> sku_sdk_loader_;

  const net::NetworkTrafficAnnotationTag& GetNetworkTrafficAnnotationTag() {
    static const net::NetworkTrafficAnnotationTag
        network_traffic_annotation_tag =
            net::DefineNetworkTrafficAnnotation("sku_sdk_execute_request", R"(
        semantics {
          sender: "Brave SKU SDK"
          description:
            "Call the SKU SDK implementation provided by the caller"
          trigger:
            "Any Brave webpage using SKU SDK where window.brave.sku.*"
            "methods are called; ex: fetch_order / fetch_order_credentials"
          data: "JSON data comprising an order."
          destination: OTHER
          destination_other: "Brave developers"
        }
        policy {
          cookies_allowed: NO
        })");
    return network_traffic_annotation_tag;
  }

  void OnFetchComplete(
      rust::cxxbridge1::Fn<
          void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>,
               brave_rewards::HttpResponse)> callback,
      rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> ctx,
      std::unique_ptr<std::string> response_body);
};

unique_ptr<SkusSdkFetcher> fetcher;

SkusSdkFetcher::SkusSdkFetcher(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory) {}

SkusSdkFetcher::~SkusSdkFetcher() {}

void SkusSdkFetcher::BeginFetch(
    const brave_rewards::HttpRequest& req,
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>,
             brave_rewards::HttpResponse)> callback,
    rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> ctx) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(ruststring_2_stdstring(req.url));
  resource_request->method = ruststring_2_stdstring(req.method).c_str();
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  // No cache read, always download from the network.
  resource_request->load_flags =
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;

  for (size_t i = 0; i < req.headers.size(); i++) {
    resource_request->headers.AddHeaderFromString(
        ruststring_2_stdstring(req.headers[i]));
  }

  sku_sdk_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), GetNetworkTrafficAnnotationTag());

  sku_sdk_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&SkusSdkFetcher::OnFetchComplete, base::Unretained(this),
                     std::move(callback), std::move(ctx)),
      kMaxResponseSize);
}

void SkusSdkFetcher::OnFetchComplete(
      rust::cxxbridge1::Fn<
          void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>,
               brave_rewards::HttpResponse)> callback,
      rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> ctx,
      std::unique_ptr<std::string> response_body) {
  if (!response_body) {
    std::vector<uint8_t> body_bytes;
    brave_rewards::HttpResponse resp = {
        RewardsResult::DecodingError,
        500,
        {},
        body_bytes,
    };
    callback(std::move(ctx), resp);
    return;
  }

  std::vector<uint8_t> body_bytes(response_body->begin(), response_body->end());

  brave_rewards::HttpResponse resp = {
      RewardsResult::Ok,
      200,
      {},
      body_bytes,
  };

  callback(std::move(ctx), resp);
}

}  // namespace

namespace brave_rewards {

void OnScheduleWakeup(rust::cxxbridge1::Fn<void()> done) {
  done();
}

void shim_scheduleWakeup(::std::uint64_t delay_ms,
                         rust::cxxbridge1::Fn<void()> done) {
  LOG(ERROR) << "shim_scheduleWakeup " << delay_ms;
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::BindOnce(&OnScheduleWakeup, std::move(done)),
      base::TimeDelta::FromMilliseconds(5000 + delay_ms));
}

void shim_executeRequest(
    const brave_rewards::HttpRequest& req,
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>,
             brave_rewards::HttpResponse)> done,
    rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> ctx) {
  fetcher = make_unique<SkusSdkFetcher>(g_SkusSdk->url_loader_factory_);
  fetcher->BeginFetch(req, std::move(done), std::move(ctx));
}

// static
void SkusSdkImpl::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(prefs::kSkusDictionary);
  registry->RegisterStringPref(prefs::kSkusVPNCredential, "");
}

SkusSdkImpl::SkusSdkImpl(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory) {
  g_SkusSdk = this;
}

// TODO: re-implement when setting preferences
// SkusSdkImpl::SkusSdkImpl(PrefService* prefs) : prefs_(prefs) {}

SkusSdkImpl::~SkusSdkImpl() {}

void on_refresh_order(RefreshOrderCallbackState* callback_state,
                      RewardsResult result,
                      rust::cxxbridge1::Str order) {
  std::string order_str = ruststr_2_stdstring(order);
  if (callback_state->cb) {
    std::move(callback_state->cb).Run(order_str);
  }
  delete callback_state;
}

void SkusSdkImpl::RefreshOrder(const std::string& order_id,
                               RefreshOrderCallback callback) {
  // TODO: properly set environment (local/dev/staging/prod)
  Box<CppSDK> sdk = initialize_sdk("local");

  std::unique_ptr<RefreshOrderCallbackState> cbs(new RefreshOrderCallbackState);
  cbs->cb = std::move(callback);

  sdk->refresh_order(on_refresh_order, std::move(cbs), order_id.c_str());
}

void SkusSdkImpl::FetchOrderCredentials(const std::string& order_id) {
  // TODO: fill me in
}

} // namespace brave_rewards
