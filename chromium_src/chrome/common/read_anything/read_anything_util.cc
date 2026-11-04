// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/common/read_anything/read_anything_util.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/containers/fixed_flat_set.h"

namespace {

// Filter out fonts that are only available from Google Fonts, and prepend
// Inter Variable (when supported for the language) so it becomes the default
// font.
void UpdateSupportedFonts(std::vector<std::string>& font_choices,
                          std::string_view language_code) {
  static constexpr auto kSupportedFonts =
      base::MakeFixedFlatSet<std::string_view>(
          {"Poppins", "Sans-serif", "Serif"});
  std::erase_if(font_choices, [](std::string_view font) {
    return !kSupportedFonts.contains(font);
  });

  static constexpr auto kInterVariableLangs =
      base::MakeFixedFlatSet<std::string_view>(
          {"af",  "bg", "ca", "cs", "da", "de", "el", "en", "es", "et", "fi",
           "fil", "fr", "hr", "hu", "id", "it", "lt", "lv", "ms", "nl", "pl",
           "pt",  "ro", "ru", "sk", "sl", "sr", "sv", "sw", "tr", "uk", "vi"});
  if (language_code.empty() || kInterVariableLangs.contains(language_code)) {
    font_choices.insert(font_choices.begin(), "Inter Variable");
  }
}

}  // namespace

#include <chrome/common/read_anything/read_anything_util.cc>
