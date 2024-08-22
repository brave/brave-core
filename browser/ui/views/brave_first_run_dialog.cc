/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_first_run_dialog.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/first_run/first_run_dialog.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/grit/branded_strings.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/gfx/font.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/window/dialog_delegate.h"

#if BUILDFLAG(ENABLE_PIN_SHORTCUT)
#include "brave/browser/brave_shell_integration.h"
#else  // BUILDFLAG(ENABLE_PIN_SHORTCUT)
#include "chrome/browser/shell_integration.h"
#endif

namespace {

void ShowBraveFirstRunDialogViews() {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  BraveFirstRunDialog::Show(run_loop.QuitClosure());
  run_loop.Run();
}

#if BUILDFLAG(ENABLE_PIN_SHORTCUT)
class PinShortcutCheckbox : public views::Checkbox {
  METADATA_HEADER(PinShortcutCheckbox, views::Checkbox)
 public:

  PinShortcutCheckbox() {
    SetFontList();
    SetText(brave_l10n::GetLocalizedResourceUTF16String(
        IDS_FIRSTRUN_DLG_PIN_SHORTCUT_TEXT));
  }
  ~PinShortcutCheckbox() override = default;
  PinShortcutCheckbox(const PinShortcutCheckbox&) = delete;
  PinShortcutCheckbox& operator=(const PinShortcutCheckbox&) = delete;

 private:
  void SetFontList() {
    gfx::FontList font_list;
    constexpr int kFontSize = 14;
    font_list.Derive(kFontSize - font_list.GetFontSize(),
                     font_list.GetFontStyle(), gfx::Font::Weight::NORMAL);
    label()->SetFontList(font_list);
  }
};

BEGIN_METADATA(PinShortcutCheckbox)
END_METADATA
#endif  // BUILDFLAG(IS_WIN)

}  // namespace

namespace first_run {

void ShowFirstRunDialog() {
#if BUILDFLAG(IS_MAC)
  if (base::FeatureList::IsEnabled(features::kViewsFirstRunDialog))
    ShowBraveFirstRunDialogViews();
  else
    ShowFirstRunDialogCocoa();
#else
  ShowBraveFirstRunDialogViews();
#endif
}

}  // namespace first_run

// static
void BraveFirstRunDialog::Show(base::RepeatingClosure quit_runloop) {
  BraveFirstRunDialog* dialog =
      new BraveFirstRunDialog(std::move(quit_runloop));
  views::DialogDelegate::CreateDialogWidget(dialog, nullptr, nullptr)->Show();
}

BraveFirstRunDialog::BraveFirstRunDialog(base::RepeatingClosure quit_runloop)
    : quit_runloop_(quit_runloop) {
  set_should_ignore_snapping(true);
#if BUILDFLAG(IS_LINUX)
  SetTitle(IDS_FIRST_RUN_DIALOG_WINDOW_TITLE);
#endif
  SetButtonLabel(ui::mojom::DialogButton::kOk,
                 brave_l10n::GetLocalizedResourceUTF16String(
                     IDS_FIRSTRUN_DLG_OK_BUTTON_LABEL));
  SetButtonLabel(ui::mojom::DialogButton::kCancel,
                 brave_l10n::GetLocalizedResourceUTF16String(
                     IDS_FIRSTRUN_DLG_CANCEL_BUTTON_LABEL));

  constexpr int kHeaderFontSize = 16;
  int size_diff =
      kHeaderFontSize - views::Label::GetDefaultFontList().GetFontSize();
  views::Label::CustomFont header_font = {
      views::Label::GetDefaultFontList()
          .DeriveWithSizeDelta(size_diff)
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD)};
  auto* header_label = AddChildView(std::make_unique<views::Label>(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_FIRSTRUN_DLG_HEADER_TEXT),
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
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_FIRSTRUN_DLG_CONTENTS_TEXT),
      contents_font));
  contents_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  contents_label->SetMultiLine(true);
  constexpr int kMaxWidth = 350;
  contents_label->SetMaximumWidth(kMaxWidth);

#if BUILDFLAG(ENABLE_PIN_SHORTCUT)
  pin_shortcut_checkbox_ =
      AddChildView(std::make_unique<PinShortcutCheckbox>());
#endif

  constexpr int kChildSpacing = 16;
  constexpr int kPadding = 24;
  constexpr int kTopPadding = 20;
  int kBottomPadding = 55;

#if BUILDFLAG(ENABLE_PIN_SHORTCUT)
  kBottomPadding -= pin_shortcut_checkbox_->GetPreferredSize().height();
#endif

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::TLBR(kTopPadding, kPadding, kBottomPadding, kPadding),
      kChildSpacing));
}

BraveFirstRunDialog::~BraveFirstRunDialog() = default;

void BraveFirstRunDialog::Done() {
  CHECK(!quit_runloop_.is_null());
  quit_runloop_.Run();
}

bool BraveFirstRunDialog::Accept() {
  GetWidget()->Hide();

#if BUILDFLAG(ENABLE_PIN_SHORTCUT)
  base::MakeRefCounted<shell_integration::BraveDefaultBrowserWorker>()
      ->StartSetAsDefault(base::BindOnce(
          [](bool pin_to_shortcut,
             shell_integration::DefaultWebClientState state) {
            if (pin_to_shortcut &&
                state == shell_integration::DefaultWebClientState::IS_DEFAULT) {
              // Try to pin to taskbar when Brave is set as a default browser.
              shell_integration::PinShortcut();
            }
          },
          pin_shortcut_checkbox_->GetChecked()));
#else
  shell_integration::SetAsDefaultBrowser();
#endif

  Done();
  return true;
}

void BraveFirstRunDialog::WindowClosing() {
  Done();
}

BEGIN_METADATA(BraveFirstRunDialog)
END_METADATA
