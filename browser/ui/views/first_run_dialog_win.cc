/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/first_run_dialog_win.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "brave/browser/brave_shell_integration.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/first_run/first_run.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/font.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/window/dialog_delegate.h"

namespace first_run {

void ShowFirstRunDialog(Profile* profile) {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  FirstRunDialogWin::Show(run_loop.QuitClosure());
  run_loop.Run();
}

}  // namespace first_run

// static
void FirstRunDialogWin::Show(base::RepeatingClosure quit_runloop) {
  FirstRunDialogWin* dialog = new FirstRunDialogWin(std::move(quit_runloop));
  views::DialogDelegate::CreateDialogWidget(dialog, NULL, NULL)->Show();
}

FirstRunDialogWin::FirstRunDialogWin(base::RepeatingClosure quit_runloop)
    : quit_runloop_(quit_runloop) {
  set_should_ignore_snapping(true);
  SetButtonLabel(
      ui::DIALOG_BUTTON_OK,
      l10n_util::GetStringUTF16(IDS_FIRSTRUN_DLG_WIN_OK_BUTTON_LABEL));
  SetButtonLabel(
      ui::DIALOG_BUTTON_CANCEL,
      l10n_util::GetStringUTF16(IDS_FIRSTRUN_DLG_WIN_CANCEL_BUTTON_LABEL));
  constexpr int kChildSpacing = 16;
  constexpr int kPadding = 24;
  constexpr int kTopPadding = 20;
  constexpr int kBottomPadding = 55;

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(kTopPadding, kPadding, kBottomPadding, kPadding),
      kChildSpacing));

  constexpr int kHeaderFontSize = 16;
  int size_diff =
      kHeaderFontSize - views::Label::GetDefaultFontList().GetFontSize();
  views::Label::CustomFont header_font = {
      views::Label::GetDefaultFontList()
          .DeriveWithSizeDelta(size_diff)
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD)};
  auto* header_label = AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_FIRSTRUN_DLG_WIN_HEADER_TEXT),
      header_font));
  header_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  constexpr int kContentFontSize = 15;
  size_diff =
      kContentFontSize - views::Label::GetDefaultFontList().GetFontSize();
  views::Label::CustomFont contents_font = {
      views::Label::GetDefaultFontList()
          .DeriveWithSizeDelta(size_diff)
          .DeriveWithWeight(gfx::Font::Weight::NORMAL)};
  auto* contents_label = AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_FIRSTRUN_DLG_WIN_CONTENTS_TEXT),
      contents_font));
  contents_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  contents_label->SetMultiLine(true);
  constexpr int kMaxWidth = 350;
  contents_label->SetMaximumWidth(kMaxWidth);
}

FirstRunDialogWin::~FirstRunDialogWin() = default;

void FirstRunDialogWin::Done() {
  CHECK(!quit_runloop_.is_null());
  quit_runloop_.Run();
}

bool FirstRunDialogWin::Accept() {
  GetWidget()->Hide();

  base::MakeRefCounted<shell_integration::BraveDefaultBrowserWorker>()
      ->StartSetAsDefault(base::NullCallback());

  Done();
  return true;
}

void FirstRunDialogWin::WindowClosing() {
  first_run::SetShouldShowWelcomePage();
  Done();
}

BEGIN_METADATA(FirstRunDialogWin, views::DialogDelegateView)
END_METADATA
