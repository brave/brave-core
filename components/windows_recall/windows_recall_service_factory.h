/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WINDOWS_RECALL_WINDOWS_RECALL_SERVICE_FACTORY_H_
#define BRAVE_COMPONENTS_WINDOWS_RECALL_WINDOWS_RECALL_SERVICE_FACTORY_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}

namespace base {

template <typename T>
class NoDestructor;
}

namespace windows_recall {

class WindowsRecallService;

class WindowsRecallServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  WindowsRecallServiceFactory(const WindowsRecallServiceFactory&) = delete;
  WindowsRecallServiceFactory& operator=(const WindowsRecallServiceFactory&) =
      delete;

  static WindowsRecallServiceFactory* GetInstance();
  static WindowsRecallService* GetForBrowserContext(
      content::BrowserContext* context);

 private:
  friend class base::NoDestructor<WindowsRecallServiceFactory>;

  WindowsRecallServiceFactory();
  ~WindowsRecallServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace windows_recall

#endif  // BRAVE_COMPONENTS_WINDOWS_RECALL_WINDOWS_RECALL_SERVICE_FACTORY_H_
