/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_
#define BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_

#include <memory>

#include "chrome/browser/themes/theme_service.h"

namespace extensions {
class BraveThemeEventRouter;
}

class BraveThemeService : public ThemeService {
 public:
  BraveThemeService(Profile* profile, const ThemeHelper& theme_helper);
  ~BraveThemeService() override;

  // ThemeService overrides:
  void Init() override;

 private:
  friend class BraveThemeServiceTestWithoutSystemTheme;
  FRIEND_TEST_ALL_PREFIXES(BraveThemeEventRouterBrowserTest, ThemeChangeTest);
  FRIEND_TEST_ALL_PREFIXES(BraveThemeServiceTest, GetBraveThemeListTest);
  FRIEND_TEST_ALL_PREFIXES(BraveThemeServiceTest, SystemThemeChangeTest);

  // Own |mock_router|.
  void SetBraveThemeEventRouterForTesting(
      extensions::BraveThemeEventRouter* mock_router);

  // Make BraveThemeService own BraveThemeEventRouter.
  // BraveThemeEventRouter does its job independently with BraveThemeService
  // because it's just native theme observer and broadcast native theme's
  // change. Its lifecycle should be tied with profile because event
  // broadcasting is done per profile.
  // I think using exsiting BraveThemeService seems fine instead of creating
  // new BrowserContextKeyedService for this.
  // Use smart ptr for testing.
  std::unique_ptr<extensions::BraveThemeEventRouter> brave_theme_event_router_;
};

#endif  // BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_
