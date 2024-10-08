/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_FILTER_LIST_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_FILTER_LIST_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/brave_shields/core/common/filter_list.mojom.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave_shields {

class FilterListService;

class FilterListServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  FilterListServiceFactory(const FilterListServiceFactory&) = delete;
  FilterListServiceFactory& operator=(const FilterListServiceFactory&) = delete;

  static mojo::PendingRemote<mojom::FilterListAndroidHandler> GetForContext(
      content::BrowserContext* context);
  static FilterListService* GetServiceForContext(
      content::BrowserContext* context);
  static FilterListServiceFactory* GetInstance();
  static void BindForContext(
      content::BrowserContext* context,
      mojo::PendingReceiver<mojom::FilterListAndroidHandler> receiver);

 private:
  friend base::NoDestructor<FilterListServiceFactory>;

  FilterListServiceFactory();
  ~FilterListServiceFactory() override;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_FILTER_LIST_SERVICE_FACTORY_H_
