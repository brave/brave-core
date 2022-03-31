/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_THEME_HELPER_H_
#define BRAVE_BROWSER_THEMES_BRAVE_THEME_HELPER_H_

#include "chrome/browser/themes/theme_helper.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class Profile;

class BraveThemeHelper : public ThemeHelper {
 public:
  BraveThemeHelper() = default;
  ~BraveThemeHelper() override;

  BraveThemeHelper(const BraveThemeHelper&) = delete;
  BraveThemeHelper& operator=(const BraveThemeHelper&) = delete;

  void set_is_tor() { is_tor_ = true; }
  void set_is_guest() { is_guest_ = true; }

 protected:
  // ThemeHelper overrides:
  SkColor GetDefaultColor(
      int id,
      bool incognito,
      const CustomThemeSupplier* theme_supplier) const override;

  absl::optional<SkColor> GetOmniboxColor(
      int id,
      bool incognito,
      const CustomThemeSupplier* theme_supplier) const override;

 private:
  bool is_tor_ = false;
  bool is_guest_ = false;
};

#endif  // BRAVE_BROWSER_THEMES_BRAVE_THEME_HELPER_H_
