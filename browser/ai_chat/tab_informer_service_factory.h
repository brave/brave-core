// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TAB_INFORMER_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_AI_CHAT_TAB_INFORMER_SERVICE_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}

namespace ai_chat {

class TabInformerService;

class TabInformerServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static TabInformerServiceFactory* GetInstance();
  static TabInformerService* GetForBrowserContext(
      content::BrowserContext* context);

 protected:
  bool ServiceIsCreatedWithBrowserContext() const override;

 private:
  friend base::NoDestructor<TabInformerServiceFactory>;

  TabInformerServiceFactory();
  ~TabInformerServiceFactory() override;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TAB_INFORMER_SERVICE_FACTORY_H_
