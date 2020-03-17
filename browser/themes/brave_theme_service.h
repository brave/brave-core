/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_
#define BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_

#include <memory>

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

namespace extensions {
class BraveThemeEventRouter;
}  // namespace extensions

class Profile;

// For now, this class exists for BraveThemeEventRouter lifecycle.
class BraveThemeService : public KeyedService {
 public:
  explicit BraveThemeService(Profile* profile);
  ~BraveThemeService() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveThemeEventRouterBrowserTest, ThemeChangeTest);

  // Own |mock_router|.
  void SetBraveThemeEventRouterForTesting(
      extensions::BraveThemeEventRouter* mock_router);

  std::unique_ptr<extensions::BraveThemeEventRouter> brave_theme_event_router_;
};

class BraveThemeServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static BraveThemeService* GetForProfile(Profile* profile);
  static BraveThemeServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BraveThemeServiceFactory>;

  BraveThemeServiceFactory();
  ~BraveThemeServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;

  DISALLOW_COPY_AND_ASSIGN(BraveThemeServiceFactory);
};

#endif  // BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_
