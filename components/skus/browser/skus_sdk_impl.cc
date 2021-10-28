// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/browser/skus_sdk_impl.h"

#include <memory>
#include <utility>

#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/skus/browser/brave-rewards-cxx/src/wrapper.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/skus_sdk_fetcher.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/preferences/public/cpp/dictionary_value_update.h"
#include "services/preferences/public/cpp/scoped_pref_update.h"
#include "url/gurl.h"

namespace {

// std::string GetEnvironment() {
//   // TODO(bsclifton): implement similar to logic to
//   // https://github.com/brave/brave-core/pull/10358/files#diff-2170e2d6e88ab6e0202eac0280482f5a45f468d0fcc8d9d4d48fc358812b4a0cR35
//   return "development";
// }

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

void OnScheduleWakeup(
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<brave_rewards::WakeupContext>)> done,
        rust::cxxbridge1::Box<brave_rewards::WakeupContext> ctx) {
  done(std::move(ctx));
}

}  // namespace

namespace brave_rewards {

void shim_purge(SkusSdkImpl& ctx) {
  ctx.PurgeStore();
}

void shim_set(SkusSdkImpl& ctx,
              rust::cxxbridge1::Str key,
              rust::cxxbridge1::Str value) {
  ctx.UpdateStoreValue(static_cast<std::string>(key),
                       static_cast<std::string>(value));
}

::rust::String shim_get(SkusSdkImpl& ctx, rust::cxxbridge1::Str key) {
  return ::rust::String(ctx.GetValueFromStore(static_cast<std::string>(key)));
}

void shim_scheduleWakeup(
    ::std::uint64_t delay_ms,
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<brave_rewards::WakeupContext>)> done,
    rust::cxxbridge1::Box<brave_rewards::WakeupContext> ctx) {
  LOG(ERROR) << "shim_scheduleWakeup " << delay_ms;
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&OnScheduleWakeup, std::move(done), std::move(ctx)),
      base::TimeDelta::FromMilliseconds(delay_ms));
}

std::unique_ptr<SkusSdkFetcher> shim_executeRequest(
    const brave_rewards::SkusSdkImpl& ctx,
    const brave_rewards::HttpRequest& req,
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>,
             brave_rewards::HttpResponse)> done,
    rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> rt_ctx) {
  auto fetcher = ctx.CreateFetcher();
  fetcher->BeginFetch(req, std::move(done), std::move(rt_ctx));
  return fetcher;
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
    : url_loader_factory_(url_loader_factory), prefs_(prefs) {
  // TODO(bsclifton): REMOVE ME (THIS IS A HACK)
  unique_instance_.reset(this);
}

SkusSdkImpl::~SkusSdkImpl() {
  // TODO(bsclifton): REMOVE ME (THIS IS A HACK)
  unique_instance_.release();
}

std::unique_ptr<SkusSdkFetcher> SkusSdkImpl::CreateFetcher() const {
  return std::make_unique<SkusSdkFetcher>(url_loader_factory_);
}

std::string SkusSdkImpl::GetValueFromStore(std::string key) const {
  LOG(ERROR) << "shim_get: `" << key << "`";
  const base::Value* dictionary = prefs_->GetDictionary(prefs::kSkusDictionary);
  DCHECK(dictionary);
  DCHECK(dictionary->is_dict());
  const base::Value* value = dictionary->FindKey(key);
  if (value) {
    return value->GetString();
  }
  return "{}";
}

void SkusSdkImpl::PurgeStore() const {
  LOG(ERROR) << "shim_purge";
  ::prefs::ScopedDictionaryPrefUpdate update(prefs_, prefs::kSkusDictionary);
  std::unique_ptr<::prefs::DictionaryValueUpdate> dictionary = update.Get();
  DCHECK(dictionary);
  dictionary->Clear();
}

void SkusSdkImpl::UpdateStoreValue(std::string key,
                                       std::string value) const {
  LOG(ERROR) << "shim_set: `" << key << "` = `" << value << "`";
  ::prefs::ScopedDictionaryPrefUpdate update(prefs_, prefs::kSkusDictionary);
  std::unique_ptr<::prefs::DictionaryValueUpdate> dictionary = update.Get();
  DCHECK(dictionary);
  dictionary->SetString(key, value);
}

void SkusSdkImpl::RefreshOrder(const std::string& order_id,
                               RefreshOrderCallback callback) {
  // TODO(bsclifton): find a better way to pass this in :(
  // basically experiencing a crash on exit
  ::rust::Box<CppSDK> sdk =
      initialize_sdk(std::move(unique_instance_), "development");

  std::unique_ptr<RefreshOrderCallbackState> cbs(new RefreshOrderCallbackState);
  cbs->cb = std::move(callback);

  sdk->refresh_order(OnRefreshOrder, std::move(cbs), order_id.c_str());
}

void SkusSdkImpl::FetchOrderCredentials(
    const std::string& order_id,
    FetchOrderCredentialsCallback callback) {
  // TODO(bsclifton): find a better way to pass this in :(
  // basically experiencing a crash on exit
  ::rust::Box<CppSDK> sdk =
      initialize_sdk(std::move(unique_instance_), "development");

  std::unique_ptr<FetchOrderCredentialsCallbackState> cbs(
      new FetchOrderCredentialsCallbackState);
  cbs->cb = std::move(callback);

  sdk->fetch_order_credentials(OnFetchOrderCredentials, std::move(cbs),
                               order_id.c_str());
}

void SkusSdkImpl::PrepareCredentialsPresentation(
    const std::string& domain,
    const std::string& path,
    PrepareCredentialsPresentationCallback callback) {
  // TODO(bsclifton): find a better way to pass this in :(
  // basically experiencing a crash on exit
  ::rust::Box<CppSDK> sdk =
      initialize_sdk(std::move(unique_instance_), "development");

  std::unique_ptr<PrepareCredentialsPresentationCallbackState> cbs(
      new PrepareCredentialsPresentationCallbackState);
  cbs->cb = std::move(callback);

  sdk->prepare_credentials_presentation(OnPrepareCredentialsPresentation,
                                        std::move(cbs), domain, path);
}

}  // namespace brave_rewards
