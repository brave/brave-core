/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "ui/views/controls/menu/menu_config.h"

constexpr int kNewEmailAliaseSuggestionMaxWidth = 512;

// This macro redefines FormatLabel at the beginning of CreateSubtextViews to
// allow long descriptions for New Email Alias suggestions to be shown.
#define GetDistanceMetric(...)                                              \
  GetDistanceMetric(__VA_ARGS__);                                           \
  auto FormatLabel_ChromiumImpl = FormatLabel;                              \
  auto FormatLabel = [&FormatLabel_ChromiumImpl, &suggestion](              \
                         views::Label& label, const Suggestion::Text& text, \
                         FillingProduct main_filling_product,               \
                         int maximum_width_single_line) {                   \
    if (suggestion.brave_new_email_alias_suggestion) {                      \
      maximum_width_single_line = kNewEmailAliaseSuggestionMaxWidth;        \
    }                                                                       \
    FormatLabel_ChromiumImpl(label, text, main_filling_product,             \
                             maximum_width_single_line);                    \
  }

#include <chrome/browser/ui/views/autofill/popup/popup_row_factory_utils.cc>

#undef GetDistanceMetric
