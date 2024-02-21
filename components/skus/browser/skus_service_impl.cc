// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/browser/skus_service_impl.h"

#include <memory>
#include <utility>

#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/rs/cxx/src/lib.rs.h"
#include "brave/components/skus/browser/skus_context_impl.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

void OnRefreshOrder(skus::RefreshOrderCallbackState* callback_state,
                    skus::SkusResult result,
                    rust::cxxbridge1::Str order) {
  if (callback_state->cb) {
    std::move(callback_state->cb).Run(static_cast<std::string>(order));
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
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs), url_loader_factory_(url_loader_factory) {
  sdk_task_runner_ = base::ThreadPool::CreateSingleThreadTaskRunner(
      {base::TaskPriority::USER_BLOCKING});
  ui_task_runner_ = base::SequencedTaskRunner::GetCurrentDefault();
}

SkusServiceImpl::~SkusServiceImpl() = default;

void SkusServiceImpl::Shutdown() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Disconnect remotes.
  receivers_.ClearWithReason(0, "Shutting down");

  for (auto it = sdks_.begin(); it != sdks_.end();) {
    // CppSDK must be destroyed on the sdk task runner.
    sdk_task_runner_->PostTask(
        FROM_HERE, base::BindOnce([](::rust::Box<skus::CppSDK> sdk) {},
                                  std::move(sdks_.extract(it++).mapped())));
  }
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::unique_ptr<skus::RefreshOrderCallbackState> cbs(
      new skus::RefreshOrderCallbackState);

  cbs->cb = base::BindOnce(
      [](mojom::SkusService::RefreshOrderCallback cb,
         scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
         const std::string& result) {
        ui_task_runner->PostTask(FROM_HERE,
                                 base::BindOnce(std::move(cb), result));
      },
      std::move(callback), ui_task_runner_);

  PostTaskWithSDK(domain,
                  base::BindOnce(
                      [](std::unique_ptr<skus::RefreshOrderCallbackState> cbs,
                         const std::string& order_id, skus::CppSDK* sdk) {
                        sdk->refresh_order(OnRefreshOrder, std::move(cbs),
                                           order_id);
                      },
                      std::move(cbs), order_id));
}

void SkusServiceImpl::FetchOrderCredentials(
    const std::string& domain,
    const std::string& order_id,
    mojom::SkusService::FetchOrderCredentialsCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::unique_ptr<skus::FetchOrderCredentialsCallbackState> cbs(
      new skus::FetchOrderCredentialsCallbackState);
  cbs->cb = base::BindOnce(
      [](mojom::SkusService::FetchOrderCredentialsCallback cb,
         scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
         const std::string& result) {
        ui_task_runner->PostTask(FROM_HERE,
                                 base::BindOnce(std::move(cb), result));
      },
      std::move(callback), ui_task_runner_);

  PostTaskWithSDK(
      domain,
      base::BindOnce(
          [](std::unique_ptr<skus::FetchOrderCredentialsCallbackState> cbs,
             const std::string& order_id, skus::CppSDK* sdk) {
            sdk->fetch_order_credentials(OnFetchOrderCredentials,
                                         std::move(cbs), order_id);
          },
          std::move(cbs), order_id));
}

void SkusServiceImpl::PrepareCredentialsPresentation(
    const std::string& domain,
    const std::string& path,
    mojom::SkusService::PrepareCredentialsPresentationCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::unique_ptr<skus::PrepareCredentialsPresentationCallbackState> cbs(
      new skus::PrepareCredentialsPresentationCallbackState);

  cbs->cb = base::BindOnce(
      [](mojom::SkusService::PrepareCredentialsPresentationCallback cb,
         scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
         const std::string& result) {
        ui_task_runner->PostTask(FROM_HERE,
                                 base::BindOnce(std::move(cb), result));
      },
      std::move(callback), ui_task_runner_);

  PostTaskWithSDK(
      domain,
      base::BindOnce(
          [](std::unique_ptr<skus::PrepareCredentialsPresentationCallbackState>
                 cbs,
             const std::string& domain, const std::string& path,
             skus::CppSDK* sdk) {
            sdk->prepare_credentials_presentation(
                OnPrepareCredentialsPresentation, std::move(cbs), domain, path);
          },
          std::move(cbs), domain, path));
}

void SkusServiceImpl::CredentialSummary(
    const std::string& domain,
    mojom::SkusService::CredentialSummaryCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::unique_ptr<skus::CredentialSummaryCallbackState> cbs(
      new skus::CredentialSummaryCallbackState);

  cbs->cb = base::BindOnce(
      [](mojom::SkusService::CredentialSummaryCallback cb,
         scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
         const std::string& result) {
        ui_task_runner->PostTask(FROM_HERE,
                                 base::BindOnce(std::move(cb), result));
      },
      std::move(callback), ui_task_runner_);

  PostTaskWithSDK(
      domain, base::BindOnce(
                  [](std::unique_ptr<skus::CredentialSummaryCallbackState> cbs,
                     const std::string& domain, skus::CppSDK* sdk) {
                    sdk->credential_summary(OnCredentialSummary, std::move(cbs),
                                            domain);
                  },
                  std::move(cbs), domain));
}

void SkusServiceImpl::SubmitReceipt(
    const std::string& domain,
    const std::string& order_id,
    const std::string& receipt,
    skus::mojom::SkusService::SubmitReceiptCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::unique_ptr<skus::SubmitReceiptCallbackState> cbs(
      new skus::SubmitReceiptCallbackState);

  cbs->cb = base::BindOnce(
      [](skus::mojom::SkusService::SubmitReceiptCallback cb,
         scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
         const std::string& result) {
        ui_task_runner->PostTask(FROM_HERE,
                                 base::BindOnce(std::move(cb), result));
      },
      std::move(callback), ui_task_runner_);

  PostTaskWithSDK(
      domain, base::BindOnce(
                  [](std::unique_ptr<skus::SubmitReceiptCallbackState> cbs,
                     const std::string& order_id, const std::string& receipt,
                     skus::CppSDK* sdk) {
                    sdk->submit_receipt(OnSubmitReceipt, std::move(cbs),
                                        order_id, receipt);
                  },
                  std::move(cbs), order_id, receipt));
}

void SkusServiceImpl::CreateOrderFromReceipt(
    const std::string& domain,
    const std::string& receipt,
    skus::mojom::SkusService::CreateOrderFromReceiptCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::unique_ptr<skus::CreateOrderFromReceiptCallbackState> cbs(
      new skus::CreateOrderFromReceiptCallbackState);

  cbs->cb = base::BindOnce(
      [](skus::mojom::SkusService::CreateOrderFromReceiptCallback cb,
         scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
         const std::string& result) {
        ui_task_runner->PostTask(FROM_HERE,
                                 base::BindOnce(std::move(cb), result));
      },
      std::move(callback), ui_task_runner_);

  PostTaskWithSDK(
      domain,
      base::BindOnce(
          [](std::unique_ptr<skus::CreateOrderFromReceiptCallbackState> cbs,
             const std::string& receipt, skus::CppSDK* sdk) {
            sdk->create_order_from_receipt(OnCreateOrderFromReceipt,
                                           std::move(cbs), receipt);
          },
          std::move(cbs), receipt));
}

void SkusServiceImpl::PurgeStore(
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StoragePurgeContext>,
                              bool success)> done,
    rust::cxxbridge1::Box<skus::StoragePurgeContext> st_ctx) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ScopedDictPrefUpdate state(&*prefs_, prefs::kSkusState);
  state->clear();

  sdk_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](rust::cxxbridge1::Fn<void(
                 rust::cxxbridge1::Box<skus::StoragePurgeContext>, bool)> done,
             rust::cxxbridge1::Box<skus::StoragePurgeContext> ctx) {
            done(std::move(ctx), true);
          },
          std::move(done), std::move(st_ctx)));
}

void SkusServiceImpl::GetValueFromStore(
    const std::string& key,
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StorageGetContext>,
                              rust::String,
                              bool)> done,
    rust::cxxbridge1::Box<skus::StorageGetContext> ctx) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const auto& state = prefs_->GetDict(prefs::kSkusState);
  const base::Value* value = state.Find(key);
  std::string result;
  if (value) {
    result = value->GetString();
  }

  sdk_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(
                     [](rust::cxxbridge1::Fn<void(
                            rust::cxxbridge1::Box<skus::StorageGetContext>,
                            rust::String, bool)> done,
                        rust::cxxbridge1::Box<skus::StorageGetContext> ctx,
                        std::string value) {
                       done(std::move(ctx), ::rust::String(value), true);
                     },
                     std::move(done), std::move(ctx), result));
}

void SkusServiceImpl::UpdateStoreValue(
    const std::string& key,
    const std::string& value,
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StorageSetContext>,
                              bool success)> done,
    rust::cxxbridge1::Box<skus::StorageSetContext> st_ctx) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ScopedDictPrefUpdate state(&*prefs_, prefs::kSkusState);
  state->Set(key, value);
  sdk_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](rust::cxxbridge1::Fn<void(
                 rust::cxxbridge1::Box<skus::StorageSetContext>, bool)> done,
             rust::cxxbridge1::Box<skus::StorageSetContext> ctx) {
            done(std::move(ctx), true);
          },
          std::move(done), std::move(st_ctx)));
}

void SkusServiceImpl::PostTaskWithSDK(
    const std::string& domain,
    base::OnceCallback<void(skus::CppSDK* sdk)> cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto env = GetEnvironmentForDomain(domain);
  if (sdks_.count(env)) {
    sdk_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(std::move(cb), &*(sdks_.at(env))));
  } else {
    sdk_task_runner_->PostTaskAndReplyWithResult(
        FROM_HERE,
        base::BindOnce(
            [](const std::string& env,
               base::WeakPtr<SkusServiceImpl> skus_service,
               std::unique_ptr<network::PendingSharedURLLoaderFactory>
                   pending_url_loader_factory,
               scoped_refptr<base::SequencedTaskRunner> ui_task_runner) {
              auto sdk =
                  initialize_sdk(std::make_unique<skus::SkusContextImpl>(
                                     std::move(pending_url_loader_factory),
                                     ui_task_runner, skus_service),
                                 env);
              return sdk;
            },
            env, weak_factory_.GetWeakPtr(), url_loader_factory_->Clone(),
            ui_task_runner_),
        base::BindOnce(&SkusServiceImpl::OnSDKInitialized,
                       weak_factory_.GetWeakPtr(), env, std::move(cb)));
  }
}

void SkusServiceImpl::OnSDKInitialized(
    const std::string& env,
    base::OnceCallback<void(skus::CppSDK* sdk)> cb,
    ::rust::Box<skus::CppSDK> sdk) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!sdks_.count(env)) {
    sdks_.insert_or_assign(env, std::move(sdk));
  }
  sdk_task_runner_->PostTask(FROM_HERE,
                             base::BindOnce(std::move(cb), &*(sdks_.at(env))));
}

}  // namespace skus
