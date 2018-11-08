/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINE_PROVIDER_SERVICE_H_
#define BRAVE_BROWSER_SEARCH_ENGINE_PROVIDER_SERVICE_H_

#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

// The purpose of this class is to configure proper search engine provider to
// private/guest/tor profile before it is referenced.
// TODO(simonhong): Migrate to KeyedService.
class SearchEngineProviderService : public content::NotificationObserver {
 public:
  SearchEngineProviderService();
  ~SearchEngineProviderService() override;

 private:
  // content::NotificationObserver overrides:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(SearchEngineProviderService);
};

#endif  // BRAVE_BROWSER_SEARCH_ENGINE_PROVIDER_SERVICE_H_
