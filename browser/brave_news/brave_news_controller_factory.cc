// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_news/brave_news_controller_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/browser/brave_news/direct_feed_fetcher_delegate_impl.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_policy/policy_initialization_waiter.h"
#include "build/build_config.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/policy/core/common/policy_service.h"
#include "content/public/browser/browser_context.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/brave_news/brave_news_controller_delegate_impl.h"
#endif

namespace brave_news {

// static
BraveNewsControllerFactory* BraveNewsControllerFactory::GetInstance() {
  static base::NoDestructor<BraveNewsControllerFactory> instance;
  return instance.get();
}

// static
BraveNewsController* BraveNewsControllerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<BraveNewsController*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
mojo::PendingRemote<mojom::BraveNewsController>
BraveNewsControllerFactory::GetRemoteForProfile(Profile* profile) {
  auto* service = static_cast<BraveNewsController*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
  if (!service) {
    return mojo::PendingRemote<mojom::BraveNewsController>();
  }

  return service->MakeRemote();
}

BraveNewsControllerFactory::BraveNewsControllerFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveNewsControllerFactory",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(HistoryServiceFactory::GetInstance());
}

BraveNewsControllerFactory::~BraveNewsControllerFactory() = default;

bool BraveNewsControllerFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

std::unique_ptr<KeyedService>
BraveNewsControllerFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!Profile::FromBrowserContext(context)->IsRegularProfile()) {
    return nullptr;
  }
  auto* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  auto* history_service = HistoryServiceFactory::GetForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);
  policy::PolicyService* policy_service = nullptr;
  if (auto* policy_connector = profile->GetProfilePolicyConnector()) {
    policy_service = policy_connector->policy_service();
  }
  auto policy_initialization_waiter =
      std::make_unique<brave_policy::PolicyInitializationWaiter>(
          policy_service);

  // The settings delegate opens the New Tab Page, which only exists on desktop;
  // a null delegate is fine, BraveNewsController::OpenSettings() no-ops then.
  std::unique_ptr<BraveNewsController::Delegate> delegate;
#if !BUILDFLAG(IS_ANDROID)
  delegate = std::make_unique<BraveNewsControllerDelegateImpl>(profile);
#endif

  return std::make_unique<BraveNewsController>(
      profile->GetPrefs(), std::move(policy_initialization_waiter),
      history_service, profile->GetURLLoaderFactory(),
      std::make_unique<DirectFeedFetcherDelegateImpl>(profile->GetPrefs()),
      std::move(delegate));
}

bool BraveNewsControllerFactory::ServiceIsNULLWhileTesting() const {
  // BraveNewsController expects non-null FaviconService, HistoryService, and
  // SharedURLLoaderFactory. All of these are nullptr in unit tests.
  return true;
}

}  // namespace brave_news
