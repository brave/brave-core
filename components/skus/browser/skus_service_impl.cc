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

namespace skus {

SkusServiceImpl::SkusServiceImpl(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs), url_loader_factory_(url_loader_factory) {
  sdk_task_runner_ = base::ThreadPool::CreateSingleThreadTaskRunner(
      {base::TaskPriority::USER_BLOCKING});
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

  PostTaskWithSDK(
      domain,
      base::BindOnce(
          [](std::unique_ptr<skus::RustBoundPostTask> callback,
             const std::string& order_id, skus::CppSDK* sdk) {
            sdk->refresh_order(std::move(callback), order_id);
          },
          std::make_unique<skus::RustBoundPostTask>(std::move(callback)),
          order_id));
}

void SkusServiceImpl::FetchOrderCredentials(
    const std::string& domain,
    const std::string& order_id,
    mojom::SkusService::FetchOrderCredentialsCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  PostTaskWithSDK(
      domain,
      base::BindOnce(
          [](std::unique_ptr<skus::RustBoundPostTask> callback,
             const std::string& order_id, skus::CppSDK* sdk) {
            sdk->fetch_order_credentials(std::move(callback), order_id);
          },
          std::make_unique<skus::RustBoundPostTask>(std::move(callback)),
          order_id));
}

void SkusServiceImpl::PrepareCredentialsPresentation(
    const std::string& domain,
    const std::string& path,
    mojom::SkusService::PrepareCredentialsPresentationCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  PostTaskWithSDK(
      domain,
      base::BindOnce(
          [](std::unique_ptr<skus::RustBoundPostTask> callback,
             const std::string& domain, const std::string& path,
             skus::CppSDK* sdk) {
            sdk->prepare_credentials_presentation(std::move(callback), domain,
                                                  path);
          },
          std::make_unique<skus::RustBoundPostTask>(std::move(callback)),
          domain, path));
}

void SkusServiceImpl::CredentialSummary(
    const std::string& domain,
    mojom::SkusService::CredentialSummaryCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  PostTaskWithSDK(
      domain,
      base::BindOnce(
          [](std::unique_ptr<skus::RustBoundPostTask> callback,
             const std::string& domain, skus::CppSDK* sdk) {
            sdk->credential_summary(std::move(callback), domain);
          },
          std::make_unique<skus::RustBoundPostTask>(std::move(callback)),
          domain));
}

void SkusServiceImpl::SubmitReceipt(
    const std::string& domain,
    const std::string& order_id,
    const std::string& receipt,
    skus::mojom::SkusService::SubmitReceiptCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  PostTaskWithSDK(
      domain,
      base::BindOnce(
          [](std::unique_ptr<skus::RustBoundPostTask> callback,
             const std::string& order_id, const std::string& receipt,
             skus::CppSDK* sdk) {
            sdk->submit_receipt(std::move(callback), order_id, receipt);
          },
          std::make_unique<skus::RustBoundPostTask>(std::move(callback)),
          order_id, receipt));
}

void SkusServiceImpl::CreateOrderFromReceipt(
    const std::string& domain,
    const std::string& receipt,
    skus::mojom::SkusService::CreateOrderFromReceiptCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  PostTaskWithSDK(
      domain,
      base::BindOnce(
          [](std::unique_ptr<skus::RustBoundPostTask> callback,
             const std::string& receipt, skus::CppSDK* sdk) {
            sdk->create_order_from_receipt(std::move(callback), receipt);
          },
          std::make_unique<skus::RustBoundPostTask>(std::move(callback)),
          receipt));
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
            base::SequencedTaskRunner::GetCurrentDefault()),
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
