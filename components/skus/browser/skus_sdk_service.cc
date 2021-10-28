// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/browser/skus_sdk_service.h"

#include <utility>

#include "base/task/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/skus/browser/skus_sdk_context.h"

namespace {

void OnRefreshOrder(brave_rewards::RefreshOrderCallbackState* callback_state,
                    brave_rewards::RewardsResult result,
                    rust::cxxbridge1::Str order) {
  std::string order_str = static_cast<std::string>(order);
  if (callback_state->cb) {
    std::move(callback_state->cb).Run(order_str);
  }
  delete callback_state;
}

void OnFetchOrderCredentials(
    brave_rewards::FetchOrderCredentialsCallbackState* callback_state,
    brave_rewards::RewardsResult result) {
  if (callback_state->cb) {
    std::move(callback_state->cb).Run("");
  }
  delete callback_state;
}

void OnPrepareCredentialsPresentation(
    brave_rewards::PrepareCredentialsPresentationCallbackState* callback_state,
    brave_rewards::RewardsResult result,
    rust::cxxbridge1::Str presentation) {
  if (callback_state->cb) {
    std::move(callback_state->cb).Run(static_cast<std::string>(presentation));
  }
  delete callback_state;
}

}  // namespace

SkusSdkService::SkusSdkService(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : context_(
          std::make_unique<brave_rewards::SkusSdkContext>(prefs,
                                                          url_loader_factory)),
      sdk_(initialize_sdk(std::move(context_), "development")),
      weak_factory_(this) {}

SkusSdkService::~SkusSdkService() {}

void SkusSdkService::RefreshOrder(
    const std::string& order_id,
    skus::mojom::SkusSdk::RefreshOrderCallback callback) {
  std::unique_ptr<brave_rewards::RefreshOrderCallbackState> cbs(
      new brave_rewards::RefreshOrderCallbackState);
  cbs->cb = std::move(callback);

  sdk_->refresh_order(OnRefreshOrder, std::move(cbs), order_id.c_str());
}

void SkusSdkService::FetchOrderCredentials(
    const std::string& order_id,
    skus::mojom::SkusSdk::FetchOrderCredentialsCallback callback) {
  std::unique_ptr<brave_rewards::FetchOrderCredentialsCallbackState> cbs(
      new brave_rewards::FetchOrderCredentialsCallbackState);
  cbs->cb = std::move(callback);

  sdk_->fetch_order_credentials(OnFetchOrderCredentials, std::move(cbs),
                                order_id.c_str());
}

void SkusSdkService::PrepareCredentialsPresentation(
    const std::string& domain,
    const std::string& path,
    skus::mojom::SkusSdk::PrepareCredentialsPresentationCallback callback) {
  std::unique_ptr<brave_rewards::PrepareCredentialsPresentationCallbackState>
      cbs(new brave_rewards::PrepareCredentialsPresentationCallbackState);
  cbs->cb = std::move(callback);

  sdk_->prepare_credentials_presentation(OnPrepareCredentialsPresentation,
                                         std::move(cbs), domain, path);
}
