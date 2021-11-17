// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/browser/skus_sdk_service.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/task/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/skus/browser/rs/cxx/src/lib.rs.h"
#include "brave/components/skus/browser/rs/cxx/src/shim.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/skus_sdk_context_impl.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "components/prefs/pref_service.h"
#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/parsed_cookie.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

void OnRefreshOrder(skus::RefreshOrderCallbackState* callback_state,
                    skus::RewardsResult result,
                    rust::cxxbridge1::Str order) {
  std::string order_str = static_cast<std::string>(order);
  if (callback_state->cb) {
    std::move(callback_state->cb).Run(order_str);
  }
  delete callback_state;
}

void OnFetchOrderCredentials(
    skus::FetchOrderCredentialsCallbackState* callback_state,
    skus::RewardsResult result) {
  if (callback_state->cb) {
    std::move(callback_state->cb).Run("");
  }
  delete callback_state;
}

void OnPrepareCredentialsPresentation(
    skus::PrepareCredentialsPresentationCallbackState* callback_state,
    skus::RewardsResult result,
    rust::cxxbridge1::Str presentation) {
  std::string credential_string = static_cast<std::string>(presentation);
  net::CookieInclusionStatus status;
  net::ParsedCookie credential_cookie(credential_string, &status);
  DCHECK(credential_cookie.IsValid());
  DCHECK(status.IsInclude());

  if (callback_state->prefs) {
    if (callback_state->domain == "vpn.brave.com" ||
        callback_state->domain == "vpn.brave.software") {
      callback_state->prefs->SetString(skus::prefs::kSkusVPNCredential,
                                       credential_cookie.Value());
    }
  }

  if (callback_state->cb) {
    std::move(callback_state->cb).Run(credential_string);
  }
  delete callback_state;
}

void OnCredentialSummary(
    skus::CredentialSummaryCallbackState* callback_state,
    skus::RewardsResult result,
    rust::cxxbridge1::Str summary) {
  std::string summary_string = static_cast<std::string>(summary);
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          summary_string, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (records_v && callback_state->prefs) {
    if (callback_state->domain == "vpn.brave.com" ||
        callback_state->domain == "vpn.brave.software") {
      const base::Value* sku = records_v->FindKey("sku");
      bool has_credential = sku && sku->is_string() &&
                            sku->GetString() == "brave-firewall-vpn-premium";
      callback_state->prefs->SetBoolean(
          skus::prefs::kSkusVPNHasCredential, has_credential);
    }
  }

  if (callback_state->cb) {
    std::move(callback_state->cb).Run(summary_string);
  }
  delete callback_state;
}

}  // namespace

SkusSdkService::SkusSdkService(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : context_(std::make_unique<skus::SkusSdkContextImpl>(
          prefs,
          url_loader_factory)),
      sdk_(
          initialize_sdk(std::move(context_), skus::GetEnvironment())),
      prefs_(prefs),
      weak_factory_(this) {}

SkusSdkService::~SkusSdkService() {}

void SkusSdkService::RefreshOrder(
    const std::string& order_id,
    skus::mojom::SkusSdk::RefreshOrderCallback callback) {
  std::unique_ptr<skus::RefreshOrderCallbackState> cbs(
      new skus::RefreshOrderCallbackState);
  cbs->cb = std::move(callback);

  sdk_->refresh_order(OnRefreshOrder, std::move(cbs), order_id.c_str());
}

void SkusSdkService::FetchOrderCredentials(
    const std::string& order_id,
    skus::mojom::SkusSdk::FetchOrderCredentialsCallback callback) {
  std::unique_ptr<skus::FetchOrderCredentialsCallbackState> cbs(
      new skus::FetchOrderCredentialsCallbackState);
  cbs->cb = std::move(callback);
  cbs->order_id = order_id;

  sdk_->fetch_order_credentials(OnFetchOrderCredentials, std::move(cbs),
                                order_id.c_str());
}

void SkusSdkService::PrepareCredentialsPresentation(
    const std::string& domain,
    const std::string& path,
    skus::mojom::SkusSdk::PrepareCredentialsPresentationCallback callback) {
  std::unique_ptr<skus::PrepareCredentialsPresentationCallbackState>
      cbs(new skus::PrepareCredentialsPresentationCallbackState);
  cbs->cb = std::move(callback);
  cbs->domain = domain;
  cbs->prefs = prefs_;

  sdk_->prepare_credentials_presentation(OnPrepareCredentialsPresentation,
                                         std::move(cbs), domain, path);
}

void SkusSdkService::CredentialSummary(
    const std::string& domain,
    skus::mojom::SkusSdk::CredentialSummaryCallback callback) {
  std::unique_ptr<skus::CredentialSummaryCallbackState> cbs(
      new skus::CredentialSummaryCallbackState);
  cbs->cb = std::move(callback);
  cbs->domain = domain;
  cbs->prefs = prefs_;

  sdk_->credential_summary(OnCredentialSummary, std::move(cbs), domain);
}
