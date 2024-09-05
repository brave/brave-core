/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_
#define BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "chrome/browser/themes/theme_service.h"

namespace extensions {
class BraveThemeEventRouter;
}  // namespace extensions

class Profile;

class BraveThemeService : public ThemeService {
 public:
  explicit BraveThemeService(Profile* profile, const ThemeHelper& theme_helper);
  ~BraveThemeService() override;

  // ThemeService:
  bool GetIsGrayscale() const override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveThemeEventRouterBrowserTest, ThemeChangeTest);

  // Own |mock_router|.
  void SetBraveThemeEventRouterForTesting(
      extensions::BraveThemeEventRouter* mock_router);

  std::unique_ptr<extensions::BraveThemeEventRouter> brave_theme_event_router_;
};

#endif  // BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_
