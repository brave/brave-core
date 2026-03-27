// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_LOCAL_AI_ON_DEVICE_SPEECH_RECOGNITION_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_LOCAL_AI_ON_DEVICE_SPEECH_RECOGNITION_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/local_ai/core/on_device_speech_recognition.mojom.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

class KeyedService;
class Profile;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace local_ai {

class OnDeviceSpeechRecognitionService;

class OnDeviceSpeechRecognitionServiceFactory
    : public ProfileKeyedServiceFactory {
 public:
  static OnDeviceSpeechRecognitionServiceFactory* GetInstance();
  static mojo::PendingRemote<mojom::OnDeviceSpeechRecognitionService>
  GetForProfile(Profile* profile);
  static void BindForProfile(
      Profile* profile,
      mojo::PendingReceiver<mojom::OnDeviceSpeechRecognitionService> receiver);

 private:
  friend base::NoDestructor<OnDeviceSpeechRecognitionServiceFactory>;
  OnDeviceSpeechRecognitionServiceFactory();
  ~OnDeviceSpeechRecognitionServiceFactory() override;

  static OnDeviceSpeechRecognitionService* GetServiceForProfile(
      Profile* profile);

  OnDeviceSpeechRecognitionServiceFactory(
      const OnDeviceSpeechRecognitionServiceFactory&) = delete;
  OnDeviceSpeechRecognitionServiceFactory& operator=(
      const OnDeviceSpeechRecognitionServiceFactory&) = delete;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace local_ai

#endif  // BRAVE_BROWSER_LOCAL_AI_ON_DEVICE_SPEECH_RECOGNITION_SERVICE_FACTORY_H_
