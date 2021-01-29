/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "ui/views/controls/button/checkbox.h"

namespace views {
// Override to call SetMultiLine().
// The label of crash report checkbox should be formatted to 2 lines.
// Otherwise, dialog width is too long.
class MultiLineCheckBox : public views::Checkbox {
 public:
  explicit MultiLineCheckBox(const base::string16& label) : Checkbox(label) {
    SetMultiLine(true);
  }
  ~MultiLineCheckBox() override = default;
  MultiLineCheckBox(const MultiLineCheckBox&) = delete;
  MultiLineCheckBox& operator=(const MultiLineCheckBox&) = delete;
};
}  // namespace views

// Replaced string here instead of by running 'npm run chromium_rebase_l10n'
// because same string is used from other IDS_XXX..
#undef IDS_FR_ENABLE_LOGGING
#undef IDS_FR_CUSTOMIZE_DEFAULT_BROWSER
#define IDS_FR_ENABLE_LOGGING IDS_FR_ENABLE_LOGGING_BRAVE
#define IDS_FR_CUSTOMIZE_DEFAULT_BROWSER IDS_FR_CUSTOMIZE_DEFAULT_BROWSER_BRAVE
#define Checkbox MultiLineCheckBox

#include "../../../../../../chrome/browser/ui/views/first_run_dialog.cc"

#undef IDS_FR_ENABLE_LOGGING
#undef IDS_FR_CUSTOMIZE_DEFAULT_BROWSER
#undef Checkbox
