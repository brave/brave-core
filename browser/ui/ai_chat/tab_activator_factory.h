// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_AI_CHAT_TAB_ACTIVATOR_FACTORY_H_
#define BRAVE_BROWSER_UI_AI_CHAT_TAB_ACTIVATOR_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace content {
class BrowserContext;
}

namespace ai_chat {

class TabActivator;

class TabActivatorFactory : public ProfileKeyedServiceFactory {
 public:
  static TabActivatorFactory* GetInstance();
  static TabActivator* GetForBrowserContext(content::BrowserContext* context);

 protected:
  bool ServiceIsCreatedWithBrowserContext() const override;

 private:
  friend base::NoDestructor<TabActivatorFactory>;

  TabActivatorFactory();
  ~TabActivatorFactory() override;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_AI_CHAT_TAB_ACTIVATOR_FACTORY_H_
