/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_WIDGET_NATIVE_WIDGET_MAC_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_WIDGET_NATIVE_WIDGET_MAC_H_

#include <optional>

#include "ui/views/widget/native_widget_private.h"

#define SetWindowIcons                                      \
  SetWindowTitleVisibility(bool visible);                   \
  bool has_overridden_window_title_visibility() const {     \
    return overridden_window_title_visibility_.has_value(); \
  }                                                         \
  bool GetOverriddenWindowTitleVisibility() const;          \
  void ResetWindowControlsPosition();                       \
  void UpdateWindowTitleColor(SkColor color);               \
                                                            \
 private:                                                   \
  std::optional<bool> overridden_window_title_visibility_;  \
                                                            \
 public:                                                    \
  void SetWindowIcons

#include "src/ui/views/widget/native_widget_mac.h"  // IWYU pragma: export

#undef SetWindowIcons

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_WIDGET_NATIVE_WIDGET_MAC_H_
