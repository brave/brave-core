/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"

#define ShowFirstRunDialog ShowFirstRunDialog_UnUsed

// Replaced string here instead of by running 'npm run chromium_rebase_l10n'
// because same string is used from other IDS_XXX..
#undef IDS_FR_ENABLE_LOGGING
#undef IDS_FR_CUSTOMIZE_DEFAULT_BROWSER
#define IDS_FR_ENABLE_LOGGING IDS_FR_ENABLE_LOGGING_BRAVE
#define IDS_FR_CUSTOMIZE_DEFAULT_BROWSER IDS_FR_CUSTOMIZE_DEFAULT_BROWSER_BRAVE
#include "../../../../../../chrome/browser/ui/views/first_run_dialog.cc"
#undef IDS_FR_ENABLE_LOGGING
#undef IDS_FR_CUSTOMIZE_DEFAULT_BROWSER
#undef ShowFirstRunDialog

// Override to call SetMultiLine(). Our above new IDS should be displayed in
// two lines.
void FirstRunDialog::ShowBrave(Profile* profile) {
  FirstRunDialog* dialog = new FirstRunDialog(profile);
  auto* report_crashes = static_cast<views::Checkbox*>(
      dialog->children()[dialog->children().size() - 1]);
  report_crashes->SetMultiLine(true);
  views::DialogDelegate::CreateDialogWidget(dialog, NULL, NULL)->Show();

  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  dialog->quit_runloop_ = run_loop.QuitClosure();
  run_loop.Run();
}

namespace first_run {

void ShowFirstRunDialog(Profile* profile) {
  FirstRunDialog::ShowBrave(profile);
}

}  // namespace first_run
