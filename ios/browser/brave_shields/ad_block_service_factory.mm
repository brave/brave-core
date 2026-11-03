// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/ad_block_service_factory.h"

#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_shields/core/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/ios/browser/brave_shields/ad_block_subscription_download_manager_factory.h"
#include "components/application_locale_storage/application_locale_storage.h"
#include "components/keyed_service/core/keyed_service.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace brave_shields {

namespace {

class AdBlockServiceHolder : public KeyedService {
 public:
  explicit AdBlockServiceHolder(std::unique_ptr<AdBlockService> service)
      : service_(std::move(service)) {}
  ~AdBlockServiceHolder() override = default;

  AdBlockService* service() { return service_.get(); }

 private:
  std::unique_ptr<AdBlockService> service_;
};

}  // namespace

// static
AdBlockService* AdBlockServiceFactory::GetForProfile(ProfileIOS* profile) {
  auto* holder = GetInstance()->GetServiceForProfileAs<AdBlockServiceHolder>(
      profile, /*create=*/true);
  return holder ? holder->service() : nullptr;
}

// static
AdBlockService* AdBlockServiceFactory::GetForProfileIfExists(
    ProfileIOS* profile) {
  auto* holder = GetInstance()->GetServiceForProfileAs<AdBlockServiceHolder>(
      profile, /*create=*/false);
  return holder ? holder->service() : nullptr;
}

// static
AdBlockServiceFactory* AdBlockServiceFactory::GetInstance() {
  static base::NoDestructor<AdBlockServiceFactory> instance;
  return instance.get();
}

AdBlockServiceFactory::AdBlockServiceFactory()
    : ProfileKeyedServiceFactoryIOS("AdBlockService",
                                    ProfileSelection::kRedirectedInIncognito,
                                    TestingCreation::kNoServiceForTests) {
  DependsOn(AdBlockSubscriptionDownloadManagerFactory::GetInstance());
}

AdBlockServiceFactory::~AdBlockServiceFactory() = default;

std::unique_ptr<KeyedService> AdBlockServiceFactory::BuildServiceInstanceFor(
    ProfileIOS* profile) const {
  if (!base::FeatureList::IsEnabled(features::kBraveIOSSharedAdBlockService)) {
    return nullptr;
  }

  auto* download_manager =
      AdBlockSubscriptionDownloadManagerFactory::GetForProfile(profile);
  auto* application_context = GetApplicationContext();
  return std::make_unique<AdBlockServiceHolder>(
      std::make_unique<AdBlockService>(
          application_context->GetLocalState(),
          std::string(application_context->GetApplicationLocaleStorage()
                          ->GetTag()
                          .tag_string()),
          application_context->GetComponentUpdateService(),
          base::ThreadPool::CreateSequencedTaskRunner(
              {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
               base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}),
          base::BindOnce(
              [](AdBlockSubscriptionDownloadManager* download_manager,
                 base::OnceCallback<void(AdBlockSubscriptionDownloadManager*)>
                     callback) { std::move(callback).Run(download_manager); },
              download_manager),
          profile->GetStatePath()));
}

}  // namespace brave_shields
