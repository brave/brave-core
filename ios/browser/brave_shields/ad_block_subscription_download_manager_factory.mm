// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/ad_block_subscription_download_manager_factory.h"

#include "base/no_destructor.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_shields/core/browser/ad_block_subscription_download_manager.h"
#include "ios/chrome/browser/download/model/background_service/background_download_service_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace brave_shields {

// static
AdBlockSubscriptionDownloadManager*
AdBlockSubscriptionDownloadManagerFactory::GetForProfile(ProfileIOS* profile) {
  return GetInstance()
      ->GetServiceForProfileAs<AdBlockSubscriptionDownloadManager>(
          profile, /*create=*/true);
}

// static
AdBlockSubscriptionDownloadManagerFactory*
AdBlockSubscriptionDownloadManagerFactory::GetInstance() {
  static base::NoDestructor<AdBlockSubscriptionDownloadManagerFactory> instance;
  return instance.get();
}

AdBlockSubscriptionDownloadManagerFactory::
    AdBlockSubscriptionDownloadManagerFactory()
    : ProfileKeyedServiceFactoryIOS("AdBlockSubscriptionDownloadManager") {
  DependsOn(BackgroundDownloadServiceFactory::GetInstance());
}

AdBlockSubscriptionDownloadManagerFactory::
    ~AdBlockSubscriptionDownloadManagerFactory() = default;

std::unique_ptr<KeyedService>
AdBlockSubscriptionDownloadManagerFactory::BuildServiceInstanceFor(
    ProfileIOS* profile) const {
  return std::make_unique<AdBlockSubscriptionDownloadManager>(
      BackgroundDownloadServiceFactory::GetForProfile(profile),
      base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT}));
}

}  // namespace brave_shields
