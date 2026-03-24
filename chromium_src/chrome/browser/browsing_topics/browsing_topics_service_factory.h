/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSING_TOPICS_BROWSING_TOPICS_SERVICE_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSING_TOPICS_BROWSING_TOPICS_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class Profile;

namespace browsing_topics {

class BrowsingTopicsService;

class BrowsingTopicsServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static BrowsingTopicsService* GetForProfile(Profile* profile);
  static BrowsingTopicsServiceFactory* GetInstance();

  BrowsingTopicsServiceFactory(const BrowsingTopicsServiceFactory&) = delete;
  BrowsingTopicsServiceFactory& operator=(const BrowsingTopicsServiceFactory&) =
      delete;
  BrowsingTopicsServiceFactory(BrowsingTopicsServiceFactory&&) = delete;
  BrowsingTopicsServiceFactory& operator=(BrowsingTopicsServiceFactory&&) =
      delete;

 private:
  friend class base::NoDestructor<BrowsingTopicsServiceFactory>;

  BrowsingTopicsServiceFactory();
  ~BrowsingTopicsServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

}  // namespace browsing_topics

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSING_TOPICS_BROWSING_TOPICS_SERVICE_FACTORY_H_
