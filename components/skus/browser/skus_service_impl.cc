// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/browser/skus_service_impl.h"

#include <memory>
#include <utility>

#include "base/json/json_reader.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/rs/cxx/src/lib.rs.h"
#include "brave/components/skus/browser/skus_context_impl.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

void OnRefreshOrder(skus::RefreshOrderCallbackState* callback_state,
                    skus::SkusResult result,
                    rust::cxxbridge1::Str order) {
  std::string order_str = static_cast<std::string>(order);
  if (callback_state->cb) {
    std::move(callback_state->cb).Run(order_str);
  }
  delete callback_state;
}

void OnFetchOrderCredentials(
    skus::FetchOrderCredentialsCallbackState* callback_state,
    skus::SkusResult result) {
  if (callback_state->cb) {
    std::string error_message;
    if (result != skus::SkusResult::Ok) {
      error_message = std::string{skus::result_to_string(result)};
    }

    std::move(callback_state->cb).Run(error_message);
  }

  delete callback_state;
}

void OnPrepareCredentialsPresentation(
    skus::PrepareCredentialsPresentationCallbackState* callback_state,
    skus::SkusResult result,
    rust::cxxbridge1::Str presentation) {
  if (callback_state->cb) {
    std::move(callback_state->cb).Run(static_cast<std::string>(presentation));
  }
  delete callback_state;
}

void OnCredentialSummary(skus::CredentialSummaryCallbackState* callback_state,
                         skus::SkusResult result,
                         rust::cxxbridge1::Str summary) {
  if (callback_state->cb) {
    std::move(callback_state->cb).Run(static_cast<std::string>(summary));
  }
  delete callback_state;
}

void OnSubmitReceipt(skus::SubmitReceiptCallbackState* callback_state,
                     skus::SkusResult result) {
  if (callback_state->cb) {
    std::move(callback_state->cb).Run("");
  }
  delete callback_state;
}

}  // namespace

namespace skus {

SkusServiceImpl::SkusServiceImpl(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs), url_loader_factory_(url_loader_factory) {}

SkusServiceImpl::~SkusServiceImpl() = default;

void SkusServiceImpl::Shutdown() {}

mojo::PendingRemote<mojom::SkusService> SkusServiceImpl::MakeRemote() {
  mojo::PendingRemote<mojom::SkusService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void SkusServiceImpl::Bind(mojo::PendingReceiver<mojom::SkusService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void SkusServiceImpl::RefreshOrder(
    const std::string& domain,
    const std::string& order_id,
    mojom::SkusService::RefreshOrderCallback callback) {
  std::unique_ptr<skus::RefreshOrderCallbackState> cbs(
      new skus::RefreshOrderCallbackState);
  cbs->cb = std::move(callback);
  GetOrCreateSDK(domain)->refresh_order(OnRefreshOrder, std::move(cbs),
                                        order_id);
}

void SkusServiceImpl::FetchOrderCredentials(
    const std::string& domain,
    const std::string& order_id,
    mojom::SkusService::FetchOrderCredentialsCallback callback) {
  std::unique_ptr<skus::FetchOrderCredentialsCallbackState> cbs(
      new skus::FetchOrderCredentialsCallbackState);
  cbs->cb = std::move(callback);
  GetOrCreateSDK(domain)->fetch_order_credentials(OnFetchOrderCredentials,
                                                  std::move(cbs), order_id);
}

void SkusServiceImpl::PrepareCredentialsPresentation(
    const std::string& domain,
    const std::string& path,
    mojom::SkusService::PrepareCredentialsPresentationCallback callback) {
  std::unique_ptr<skus::PrepareCredentialsPresentationCallbackState> cbs(
      new skus::PrepareCredentialsPresentationCallbackState);
  cbs->cb = std::move(callback);
  GetOrCreateSDK(domain)->prepare_credentials_presentation(
      OnPrepareCredentialsPresentation, std::move(cbs), domain, path);
}

::rust::Box<skus::CppSDK>& SkusServiceImpl::GetOrCreateSDK(
    const std::string& domain) {
  auto env = GetEnvironmentForDomain(domain);
  if (sdk_.count(env)) {
    return sdk_.at(env);
  }

  auto sdk = initialize_sdk(
      std::make_unique<skus::SkusContextImpl>(prefs_, url_loader_factory_),
      env);
  sdk_.insert_or_assign(env, std::move(sdk));
  return sdk_.at(env);
}

void SkusServiceImpl::CredentialSummary(
    const std::string& domain,
    mojom::SkusService::CredentialSummaryCallback callback) {
  std::unique_ptr<skus::CredentialSummaryCallbackState> cbs(
      new skus::CredentialSummaryCallbackState);
  cbs->cb =
      base::BindOnce(&SkusServiceImpl::OnCredentialSummary,
                     weak_factory_.GetWeakPtr(), domain, std::move(callback));

  GetOrCreateSDK(domain)->credential_summary(::OnCredentialSummary,
                                             std::move(cbs), domain);
}

void SkusServiceImpl::OnCredentialSummary(
    const std::string& domain,
    mojom::SkusService::CredentialSummaryCallback callback,
    const std::string& summary_string) {
  if (callback) {
    std::move(callback).Run(summary_string);
  }
}

void SkusServiceImpl::SubmitReceipt(
    const std::string& domain,
    const std::string& order_id,
    const std::string& receipt,
    skus::mojom::SkusService::SubmitReceiptCallback callback) {
  std::unique_ptr<skus::SubmitReceiptCallbackState> cbs(
      new skus::SubmitReceiptCallbackState);
  cbs->cb = std::move(callback);
  GetOrCreateSDK(domain)->submit_receipt(OnSubmitReceipt, std::move(cbs),
                                         order_id, receipt);
}

}  // namespace skus
