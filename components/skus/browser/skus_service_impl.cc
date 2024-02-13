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

void OnCreateOrderFromReceipt(
    skus::CreateOrderFromReceiptCallbackState* callback_state,
    skus::SkusResult result,
    rust::cxxbridge1::Str order_id) {
  if (callback_state->cb) {
    std::move(callback_state->cb).Run(static_cast<std::string>(order_id));
  }
  delete callback_state;
}

}  // namespace

namespace skus {

SkusServiceImpl::SkusServiceImpl(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    scoped_refptr<base::SingleThreadTaskRunner> sdk_task_runner,
    scoped_refptr<base::SequencedTaskRunner> ui_task_runner)
    : prefs_(prefs),
      url_loader_factory_(url_loader_factory),
      sdk_task_runner_(sdk_task_runner),
      ui_task_runner_(ui_task_runner) {}

SkusServiceImpl::~SkusServiceImpl() = default;

void SkusServiceImpl::Shutdown() {
  sdk_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&SkusServiceImpl::CleanupSDK, base::Unretained(this)));
}

void SkusServiceImpl::CleanupSDK() {
  sdk_.clear();
}

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
  sdk_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&SkusServiceImpl::RefreshOrderTask,
                                base::Unretained(this), domain, order_id,
                                std::move(cbs), url_loader_factory_->Clone()));
}

void SkusServiceImpl::RefreshOrderTask(
    const std::string& domain,
    const std::string& order_id,
    std::unique_ptr<skus::RefreshOrderCallbackState> cbs,
    std::unique_ptr<network::PendingSharedURLLoaderFactory>
        pending_url_loader_factory) {
  GetOrCreateSDK(domain, std::move(pending_url_loader_factory))
      ->refresh_order(OnRefreshOrder, std::move(cbs), order_id);
}

void SkusServiceImpl::FetchOrderCredentials(
    const std::string& domain,
    const std::string& order_id,
    mojom::SkusService::FetchOrderCredentialsCallback callback) {
  std::unique_ptr<skus::FetchOrderCredentialsCallbackState> cbs(
      new skus::FetchOrderCredentialsCallbackState);
  cbs->cb = std::move(callback);
  sdk_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&SkusServiceImpl::FetchOrderCredentialsTask,
                                base::Unretained(this), domain, order_id,
                                std::move(cbs), url_loader_factory_->Clone()));
}

void SkusServiceImpl::FetchOrderCredentialsTask(
    const std::string& domain,
    const std::string& order_id,
    std::unique_ptr<skus::FetchOrderCredentialsCallbackState> cbs,
    std::unique_ptr<network::PendingSharedURLLoaderFactory>
        pending_url_loader_factory) {
  GetOrCreateSDK(domain, std::move(pending_url_loader_factory))
      ->fetch_order_credentials(OnFetchOrderCredentials, std::move(cbs),
                                order_id);
}

void SkusServiceImpl::PrepareCredentialsPresentation(
    const std::string& domain,
    const std::string& path,
    mojom::SkusService::PrepareCredentialsPresentationCallback callback) {
  std::unique_ptr<skus::PrepareCredentialsPresentationCallbackState> cbs(
      new skus::PrepareCredentialsPresentationCallbackState);
  cbs->cb = std::move(callback);
  sdk_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&SkusServiceImpl::PrepareCredentialsPresentationTask,
                     base::Unretained(this), domain, path, std::move(cbs),
                     url_loader_factory_->Clone()));
}

void SkusServiceImpl::PrepareCredentialsPresentationTask(
    const std::string& domain,
    const std::string& path,
    std::unique_ptr<skus::PrepareCredentialsPresentationCallbackState> cbs,
    std::unique_ptr<network::PendingSharedURLLoaderFactory>
        pending_url_loader_factory) {
  GetOrCreateSDK(domain, std::move(pending_url_loader_factory))
      ->prepare_credentials_presentation(OnPrepareCredentialsPresentation,
                                         std::move(cbs), domain, path);
}

::rust::Box<skus::CppSDK>& SkusServiceImpl::GetOrCreateSDK(
    const std::string& domain,
    std::unique_ptr<network::PendingSharedURLLoaderFactory>
        pending_url_loader_factory) {
  auto env = GetEnvironmentForDomain(domain);
  if (sdk_.count(env)) {
    return sdk_.at(env);
  }

  auto sdk = initialize_sdk(std::make_unique<skus::SkusContextImpl>(
                                prefs_,
                                network::SharedURLLoaderFactory::Create(
                                    std::move(pending_url_loader_factory)),
                                sdk_task_runner_, ui_task_runner_),
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
  sdk_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&SkusServiceImpl::CredentialSummaryTask,
                                base::Unretained(this), domain, std::move(cbs),
                                url_loader_factory_->Clone()));
}

void SkusServiceImpl::CredentialSummaryTask(
    const std::string& domain,
    std::unique_ptr<skus::CredentialSummaryCallbackState> cbs,
    std::unique_ptr<network::PendingSharedURLLoaderFactory>
        pending_url_loader_factory) {
  GetOrCreateSDK(domain, std::move(pending_url_loader_factory))
      ->credential_summary(::OnCredentialSummary, std::move(cbs), domain);
}

void SkusServiceImpl::OnCredentialSummary(
    const std::string& domain,
    mojom::SkusService::CredentialSummaryCallback callback,
    const std::string& summary_string) {
  if (callback) {
    ui_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](mojom::SkusService::CredentialSummaryCallback callback,
               const std::string& summary) {
              std::move(callback).Run(summary);
            },
            std::move(callback), summary_string));
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
  sdk_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&SkusServiceImpl::SubmitReceiptTask,
                     base::Unretained(this), domain, order_id, receipt,
                     std::move(cbs), url_loader_factory_->Clone()));
}

void SkusServiceImpl::SubmitReceiptTask(
    const std::string& domain,
    const std::string& order_id,
    const std::string& receipt,
    std::unique_ptr<skus::SubmitReceiptCallbackState> cbs,
    std::unique_ptr<network::PendingSharedURLLoaderFactory>
        pending_url_loader_factory) {
  GetOrCreateSDK(domain, std::move(pending_url_loader_factory))
      ->submit_receipt(OnSubmitReceipt, std::move(cbs), order_id, receipt);
}

void SkusServiceImpl::CreateOrderFromReceipt(
    const std::string& domain,
    const std::string& receipt,
    skus::mojom::SkusService::CreateOrderFromReceiptCallback callback) {
  std::unique_ptr<skus::CreateOrderFromReceiptCallbackState> cbs(
      new skus::CreateOrderFromReceiptCallbackState);
  cbs->cb = std::move(callback);
  sdk_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&SkusServiceImpl::CreateOrderFromReceiptTask,
                                base::Unretained(this), domain, receipt,
                                std::move(cbs), url_loader_factory_->Clone()));
}

void SkusServiceImpl::CreateOrderFromReceiptTask(
    const std::string& domain,
    const std::string& receipt,
    std::unique_ptr<skus::CreateOrderFromReceiptCallbackState> cbs,
    std::unique_ptr<network::PendingSharedURLLoaderFactory>
        pending_url_loader_factory) {
  GetOrCreateSDK(domain, std::move(pending_url_loader_factory))
      ->create_order_from_receipt(::OnCreateOrderFromReceipt, std::move(cbs),
                                  receipt);
}

}  // namespace skus
