/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/ad_block_subscription_download_manager_getter.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/browser/brave_browser_process.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/download/background_download_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_key.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_manager_observer.h"
#include "components/keyed_service/core/simple_dependency_manager.h"
#include "components/keyed_service/core/simple_keyed_service_factory.h"
#include "content/public/browser/browser_context.h"

using brave_shields::AdBlockSubscriptionDownloadManager;

namespace {

class AdBlockSubscriptionDownloadManagerFactory
    : public SimpleKeyedServiceFactory {
 public:
  static AdBlockSubscriptionDownloadManagerFactory* GetInstance() {
    return base::Singleton<AdBlockSubscriptionDownloadManagerFactory>::get();
  }

  static AdBlockSubscriptionDownloadManager* GetForKey(SimpleFactoryKey* key) {
    return static_cast<AdBlockSubscriptionDownloadManager*>(
        GetInstance()->GetServiceForKey(key, true));
  }

 private:
  friend struct base::DefaultSingletonTraits<
      AdBlockSubscriptionDownloadManagerFactory>;

  AdBlockSubscriptionDownloadManagerFactory()
      : SimpleKeyedServiceFactory("AdBlockSubscriptionDownloadManagerFactory",
                                  SimpleDependencyManager::GetInstance()) {
    DependsOn(BackgroundDownloadServiceFactory::GetInstance());
  }

  ~AdBlockSubscriptionDownloadManagerFactory() override = default;

  // SimpleKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      SimpleFactoryKey* key) const override {
    return std::make_unique<AdBlockSubscriptionDownloadManager>(
        BackgroundDownloadServiceFactory::GetForKey(key),
        base::ThreadPool::CreateSequencedTaskRunner(
            {base::MayBlock(), base::TaskPriority::BEST_EFFORT}));
  }

  SimpleFactoryKey* GetKeyToUse(SimpleFactoryKey* key) const override {
    return key;
  }

  DISALLOW_COPY_AND_ASSIGN(AdBlockSubscriptionDownloadManagerFactory);
};

class AdBlockSubscriptionDownloadManagerGetterImpl
    : public ProfileManagerObserver {
 public:
  AdBlockSubscriptionDownloadManagerGetterImpl(
      base::OnceCallback<void(AdBlockSubscriptionDownloadManager*)> callback)
      : callback_(std::move(callback)) {
    auto* profile_manager = g_browser_process->profile_manager();
    auto* profile = profile_manager->GetProfileByPath(
        profile_manager->user_data_dir().Append(
            profile_manager->GetInitialProfileDir()));
    if (profile) {
      GetDownloadManager(profile);
    } else {
      g_browser_process->profile_manager()->AddObserver(this);
    }
  }

 private:
  void OnProfileAdded(Profile* profile) override {
    auto* profile_manager = g_browser_process->profile_manager();
    if (profile->GetPath() == profile_manager->user_data_dir().Append(
                                  profile_manager->GetInitialProfileDir())) {
      profile_manager->RemoveObserver(this);
      GetDownloadManager(profile);
    }
  }

  void GetDownloadManager(Profile* profile) {
    auto* download_manager =
        AdBlockSubscriptionDownloadManagerFactory::GetInstance()->GetForKey(
            profile->GetProfileKey());
    std::move(callback_).Run(download_manager);
    delete this;
  }

  base::OnceCallback<void(AdBlockSubscriptionDownloadManager*)> callback_;
};

}  // namespace

// static
AdBlockSubscriptionDownloadManager::DownloadManagerGetter
AdBlockSubscriptionDownloadManagerGetter() {
  return base::BindOnce(
      [](base::OnceCallback<void(AdBlockSubscriptionDownloadManager*)>
             callback) {
        // self deletes when the download manager is retrieved
        new AdBlockSubscriptionDownloadManagerGetterImpl(std::move(callback));
      });
}
