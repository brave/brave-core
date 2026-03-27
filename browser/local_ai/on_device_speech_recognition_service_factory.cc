// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/local_ai/on_device_speech_recognition_service_factory.h"

#include <memory>
#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/local_ai/bwc_factory_util.h"
#include "brave/components/local_ai/core/on_device_speech_recognition_service.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"

namespace local_ai {

// static
OnDeviceSpeechRecognitionServiceFactory*
OnDeviceSpeechRecognitionServiceFactory::GetInstance() {
  static base::NoDestructor<OnDeviceSpeechRecognitionServiceFactory> instance;
  return instance.get();
}

// static
mojo::PendingRemote<mojom::OnDeviceSpeechRecognitionService>
OnDeviceSpeechRecognitionServiceFactory::GetForProfile(Profile* profile) {
  auto* service = GetServiceForProfile(profile);
  if (!service) {
    return {};
  }
  return service->MakeRemote();
}

// static
void OnDeviceSpeechRecognitionServiceFactory::BindForProfile(
    Profile* profile,
    mojo::PendingReceiver<mojom::OnDeviceSpeechRecognitionService> receiver) {
  auto* service = GetServiceForProfile(profile);
  if (service) {
    service->Bind(std::move(receiver));
  }
}

// static
OnDeviceSpeechRecognitionService*
OnDeviceSpeechRecognitionServiceFactory::GetServiceForProfile(
    Profile* profile) {
  return static_cast<OnDeviceSpeechRecognitionService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

OnDeviceSpeechRecognitionServiceFactory::
    OnDeviceSpeechRecognitionServiceFactory()
    : ProfileKeyedServiceFactory(
          "OnDeviceSpeechRecognitionService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) {}

OnDeviceSpeechRecognitionServiceFactory::
    ~OnDeviceSpeechRecognitionServiceFactory() = default;

std::unique_ptr<KeyedService>
OnDeviceSpeechRecognitionServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto factory =
      MakeBWCFactory(context, kUntrustedOnDeviceSpeechRecognitionWorkerURL,
                     IDS_ON_DEVICE_SPEECH_RECOGNITION_TASK_MANAGER_TITLE);

  return std::make_unique<OnDeviceSpeechRecognitionService>(std::move(factory));
}

}  // namespace local_ai
