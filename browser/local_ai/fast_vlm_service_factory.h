// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_LOCAL_AI_FAST_VLM_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_LOCAL_AI_FAST_VLM_SERVICE_FACTORY_H_

#include <memory>

#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace local_ai {
class FastVLMService;

class FastVLMServiceFactory : public ProfileKeyedServiceFactory {
 public:
  FastVLMServiceFactory(const FastVLMServiceFactory&) = delete;
  FastVLMServiceFactory& operator=(const FastVLMServiceFactory&) = delete;

  static FastVLMServiceFactory* GetInstance();
  static FastVLMService* GetForBrowserContext(content::BrowserContext* context);

 private:
  friend base::NoDestructor<FastVLMServiceFactory>;

  FastVLMServiceFactory();
  ~FastVLMServiceFactory() override;

  // ProfileKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace local_ai

#endif  // BRAVE_BROWSER_LOCAL_AI_FAST_VLM_SERVICE_FACTORY_H_
