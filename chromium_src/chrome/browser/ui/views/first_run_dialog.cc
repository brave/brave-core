/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/first_run_dialog.h"

#include <string>

#include "base/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/first_run/first_run_dialog.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/link.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_delegate.h"

#if BUILDFLAG(IS_WIN)
#include "brave/browser/brave_shell_integration.h"
#endif

namespace first_run {

void ShowFirstRunDialog(Profile* profile) {
#if BUILDFLAG(IS_MAC)
  if (base::FeatureList::IsEnabled(features::kViewsFirstRunDialog))
    ShowFirstRunDialogViews(profile);
  else
    ShowFirstRunDialogCocoa(profile);
#else
  ShowFirstRunDialogViews(profile);
#endif
}

void ShowFirstRunDialogViews(Profile* profile) {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  FirstRunDialog::Show(
      base::BindRepeating(&platform_util::OpenExternal,
                          base::Unretained(profile),
                          GURL(chrome::kLearnMoreReportingURL)),
      run_loop.QuitClosure());
  run_loop.Run();
}

}  // namespace first_run

// static
void FirstRunDialog::Show(base::RepeatingClosure learn_more_callback,
                          base::RepeatingClosure quit_runloop) {
  FirstRunDialog* dialog = new FirstRunDialog(std::move(learn_more_callback),
                                              std::move(quit_runloop));
  views::DialogDelegate::CreateDialogWidget(dialog, NULL, NULL)->Show();
}

FirstRunDialog::FirstRunDialog(base::RepeatingClosure learn_more_callback,
                               base::RepeatingClosure quit_runloop)
    : quit_runloop_(quit_runloop) {
  // ALLOW_UNUSED_LOCAL has been removed and [[maybe_unused]] can only be used
  // alongside declarations, so reference it here just to silence the compiler's
  // -Wunused errors without having to override the header file.
  if (report_crashes_)
    report_crashes_->GetChecked();

  SetTitle(l10n_util::GetStringUTF16(IDS_FIRSTRUN_DIALOG_WINDOW_TITLE_BRAVE));
  SetButtons(ui::DIALOG_BUTTON_OK);
  SetExtraView(
      std::make_unique<views::Link>(l10n_util::GetStringUTF16(IDS_LEARN_MORE)))
      ->SetCallback(std::move(learn_more_callback));

  constexpr int kChildSpacing = 16;
  constexpr int kPadding = 24;

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(kPadding, kPadding, kPadding, kPadding), kChildSpacing));

  constexpr int kFontSize = 15;
  int size_diff = kFontSize - views::Label::GetDefaultFontList().GetFontSize();
  views::Label::CustomFont contents_font = {
      views::Label::GetDefaultFontList()
          .DeriveWithSizeDelta(size_diff)
          .DeriveWithWeight(gfx::Font::Weight::NORMAL)};
  auto* contents_label = AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(
          IDS_FIRSTRUN_DLG_COMPLETE_INSTALLATION_LABEL_BRAVE),
      contents_font));
  contents_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  contents_label->SetMultiLine(true);
  constexpr int kMaxWidth = 450;
  contents_label->SetMaximumWidth(kMaxWidth);

  make_default_ = AddChildView(std::make_unique<views::Checkbox>(
      l10n_util::GetStringUTF16(IDS_FR_CUSTOMIZE_DEFAULT_BROWSER_BRAVE)));
  make_default_->SetChecked(true);

  chrome::RecordDialogCreation(chrome::DialogIdentifier::FIRST_RUN_DIALOG);
}

FirstRunDialog::~FirstRunDialog() = default;

void FirstRunDialog::Done() {
  CHECK(!quit_runloop_.is_null());
  quit_runloop_.Run();
}

bool FirstRunDialog::Accept() {
  GetWidget()->Hide();

  if (make_default_->GetChecked()) {
    // shell_integration::SetAsDefaultBrowser() doesn't work on Windows 8+.
    // Upstream will use DefaultBrowserWorker when it's available on all OSs.
    // See the comments of shell_integration::SetAsDefaultBrowser().
#if BUILDFLAG(IS_WIN)
    base::MakeRefCounted<shell_integration::BraveDefaultBrowserWorker>()
        ->StartSetAsDefault(base::NullCallback());
#else
    shell_integration::SetAsDefaultBrowser();
#endif
  }

  Done();
  return true;
}

void FirstRunDialog::WindowClosing() {
  first_run::SetShouldShowWelcomePage();
  Done();
}

BEGIN_METADATA(FirstRunDialog, views::DialogDelegateView)
END_METADATA
