// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/on_device_ai/on_device_ai_service_factory.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "brave/components/on_device_ai/content/background_web_contents_impl.h"
#include "brave/components/on_device_ai/core/background_web_contents.h"
#include "brave/components/on_device_ai/core/on_device_ai_service.h"
#include "brave/components/on_device_ai/core/url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "chrome/browser/task_manager/web_contents_tags.h"
#include "url/gurl.h"

namespace on_device_ai {

// static
OnDeviceAIServiceFactory* OnDeviceAIServiceFactory::GetInstance() {
  static base::NoDestructor<OnDeviceAIServiceFactory> instance;
  return instance.get();
}

// static
mojo::PendingRemote<mojom::OnDeviceAIService>
OnDeviceAIServiceFactory::GetForProfile(Profile* profile) {
  auto* service = GetServiceForProfile(profile);
  if (!service) {
    return {};
  }
  return service->MakeRemote();
}

// static
void OnDeviceAIServiceFactory::BindForProfile(
    Profile* profile,
    mojo::PendingReceiver<mojom::OnDeviceAIService> receiver) {
  auto* service = GetServiceForProfile(profile);
  if (service) {
    service->Bind(std::move(receiver));
  }
}

// static
OnDeviceAIService* OnDeviceAIServiceFactory::GetServiceForProfile(
    Profile* profile) {
  return static_cast<OnDeviceAIService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

OnDeviceAIServiceFactory::OnDeviceAIServiceFactory()
    : ProfileKeyedServiceFactory(
          "OnDeviceAIService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) {}

OnDeviceAIServiceFactory::~OnDeviceAIServiceFactory() = default;

std::unique_ptr<KeyedService>
OnDeviceAIServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto factory = base::BindRepeating(
      [](content::BrowserContext* browser_context,
         BackgroundWebContents::Delegate* delegate)
          -> std::unique_ptr<BackgroundWebContents> {
        return std::make_unique<BackgroundWebContentsImpl>(
            browser_context, GURL(kUntrustedOnDeviceAIURL), delegate,
            base::BindOnce([](content::WebContents* web_contents) {
              task_manager::WebContentsTags::CreateForToolContents(
                  web_contents, IDS_ON_DEVICE_AI_TASK_MANAGER_TITLE);
            }));
      },
      context);

  return std::make_unique<OnDeviceAIService>(std::move(factory));
}

}  // namespace on_device_ai
