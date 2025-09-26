// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TAB_UI_HELPER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TAB_UI_HELPER_H_

#include <optional>
#include <string>

// Add methods to manage custom tab title which overrides the page title.
#define GetTitle(...)                                              \
  GetTitle_ChromiumImpl(__VA_ARGS__) const;                        \
  void SetCustomTitle(const std::optional<std::u16string>& title); \
  bool has_custom_title() const {                                  \
    return custom_title_.has_value();                              \
  }                                                                \
  void UpdateLastOrigin();                                         \
                                                                   \
 private:                                                          \
  std::optional<std::u16string> custom_title_;                     \
  std::optional<url::Origin> last_origin_;                         \
                                                                   \
 public:                                                           \
  std::u16string GetTitle(__VA_ARGS__)

// Add methods to manage custom emoji favicon override.
#define GetFavicon(...)                                                 \
  GetFavicon_ChromiumImpl(__VA_ARGS__) const;                           \
  void SetCustomEmojiFavicon(const std::optional<std::u16string>&);     \
  bool has_custom_emoji_favicon() const {                                \
    return custom_emoji_favicon_.has_value();                            \
  }                                                                      \
  std::optional<std::u16string> GetCustomEmojiFaviconString() const;     \
  ui::ImageModel GetEmojiFaviconImage() const;                           \
                                                                         \
 private:                                                                \
  std::optional<std::u16string> custom_emoji_favicon_;                   \
                                                                         \
 public:                                                                 \
  ui::ImageModel GetFavicon(__VA_ARGS__)

#include <chrome/browser/ui/tab_ui_helper.h>  // IWYU pragma: export

#undef GetTitle
#undef GetFavicon

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TAB_UI_HELPER_H_
