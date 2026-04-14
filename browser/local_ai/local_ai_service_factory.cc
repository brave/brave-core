// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/local_ai/local_ai_service_factory.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "brave/components/local_ai/content/background_web_contents_impl.h"
#include "brave/components/local_ai/core/local_ai_service.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "chrome/browser/task_manager/web_contents_tags.h"

namespace local_ai {

namespace {

// Called after WebContents is created but before navigation. Registers
// the mojo bind callback and tags the contents for the task manager.
void OnWebContentsCreated(LocalAIServiceFactory::BindCallback bind_cb,
                          content::WebContents* web_contents) {
  LocalAIServiceFactory::SetBindCallbackForWebContents(web_contents,
                                                       std::move(bind_cb));
  task_manager::WebContentsTags::CreateForToolContents(
      web_contents, IDS_LOCAL_AI_TASK_MANAGER_TITLE);
}

// Called when the guest profile is ready. Creates the
// BackgroundWebContentsImpl on the guest OTR profile.
void OnGuestProfileCreated(
    base::WeakPtr<LocalAIService> weak_service,
    LocalAIServiceFactory::BindCallback bind_cb,
    LocalAIService::BackgroundWebContentsCreatedCallback callback,
    Profile* guest_profile) {
  CHECK(guest_profile);
  if (!weak_service) {
    return;
  }
  auto* otr = guest_profile->GetPrimaryOTRProfile(/*create_if_needed=*/true);
  auto contents = std::make_unique<BackgroundWebContentsImpl>(
      otr, GURL(kUntrustedLocalAIURL), weak_service.get(),
      base::BindOnce(&OnWebContentsCreated, std::move(bind_cb)));
  std::move(callback).Run(std::move(contents));
}

// Factory function that creates a BackgroundWebContents on the guest
// OTR profile. Passed to LocalAIService as the
// BackgroundWebContentsFactory.
void CreateBackgroundWebContents(
    BackgroundWebContents::Delegate* delegate,
    LocalAIService::BackgroundWebContentsCreatedCallback callback) {
  auto* service = static_cast<LocalAIService*>(delegate);
  auto weak_service = service->GetWeakPtr();
  auto bind_cb = base::BindRepeating(&LocalAIService::Bind, weak_service);
  auto* profile_manager = g_browser_process->profile_manager();
  CHECK(profile_manager);
  profile_manager->CreateProfileAsync(
      ProfileManager::GetGuestProfilePath(),
      base::BindOnce(&OnGuestProfileCreated, std::move(weak_service),
                     std::move(bind_cb), std::move(callback)));
}

}  // namespace

// static
LocalAIServiceFactory* LocalAIServiceFactory::GetInstance() {
  static base::NoDestructor<LocalAIServiceFactory> instance;
  return instance.get();
}

// static
mojo::PendingRemote<mojom::LocalAIService> LocalAIServiceFactory::GetForProfile(
    Profile* profile) {
  auto* service = GetServiceForProfile(profile);
  if (!service) {
    return {};
  }
  return service->MakeRemote();
}

// static
void LocalAIServiceFactory::BindForProfile(
    Profile* profile,
    mojo::PendingReceiver<mojom::LocalAIService> receiver) {
  auto* service = GetServiceForProfile(profile);
  if (service) {
    service->Bind(std::move(receiver));
  }
}

// static
LocalAIService* LocalAIServiceFactory::GetServiceForProfile(Profile* profile) {
  return static_cast<LocalAIService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

LocalAIServiceFactory::LocalAIServiceFactory()
    : ProfileKeyedServiceFactory(
          "LocalAIService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) {}

LocalAIServiceFactory::~LocalAIServiceFactory() = default;

// static
void LocalAIServiceFactory::SetBindCallbackForWebContents(
    content::WebContents* web_contents,
    BindCallback callback) {
  GetInstance()->web_contents_bind_callbacks_[web_contents] =
      std::move(callback);
}

// static
void LocalAIServiceFactory::RemoveBindCallbackForWebContents(
    content::WebContents* web_contents) {
  GetInstance()->web_contents_bind_callbacks_.erase(web_contents);
}

// static
void LocalAIServiceFactory::BindForWebContents(
    content::WebContents* web_contents,
    mojo::PendingReceiver<mojom::LocalAIService> receiver) {
  auto* instance = GetInstance();
  auto it = instance->web_contents_bind_callbacks_.find(web_contents);
  if (it != instance->web_contents_bind_callbacks_.end()) {
    it->second.Run(std::move(receiver));
  }
}

std::unique_ptr<KeyedService>
LocalAIServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<LocalAIService>(
      base::BindRepeating(&CreateBackgroundWebContents),
      LocalModelsUpdaterState::GetInstance());
}

}  // namespace local_ai
