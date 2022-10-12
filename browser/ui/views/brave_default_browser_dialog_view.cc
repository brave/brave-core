/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_default_browser_dialog_view.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/scoped_refptr.h"
#include "brave/browser/brave_shell_integration.h"
#include "brave/browser/ui/browser_dialogs.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/prefs/pref_service.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/layout_provider.h"

#if BUILDFLAG(IS_WIN)
#include "brave/browser/brave_shell_integration_win.h"
#endif

namespace brave {

void ShowDefaultBrowserDialog(Browser* browser) {
  constrained_window::CreateBrowserModalDialogViews(
      new BraveDefaultBrowserDialogView(), browser->window()->GetNativeWindow())
      ->Show();
}

}  // namespace brave

namespace {

constexpr int kPadding = 24;

class DontAskAgainButton : public views::LabelButton {
 public:
  METADATA_HEADER(DontAskAgainButton);

  explicit DontAskAgainButton(PressedCallback callback)
      : LabelButton(std::move(callback)) {
    SetFontList();
    SetText(brave_l10n::GetLocalizedResourceUTF16String(
        IDS_BRAVE_DEFAULT_BROWSER_DIALOG_DONT_ASK));
  }
  DontAskAgainButton(const DontAskAgainButton&) = delete;
  DontAskAgainButton& operator=(const DontAskAgainButton&) = delete;
  ~DontAskAgainButton() override = default;

 private:
  void SetFontList() {
    gfx::FontList font_list;
    constexpr int kFontSize = 13;
    font_list.Derive(kFontSize - font_list.GetFontSize(),
                     font_list.GetFontStyle(), gfx::Font::Weight::NORMAL);
    label()->SetFontList(font_list);
  }

  // views::LabelButton overrides:
  void OnThemeChanged() override {
    LabelButton::OnThemeChanged();

    auto* cp = GetColorProvider();
    SetTextColor(views::Button::STATE_NORMAL,
                 cp->GetColor(kColorDialogDontAskAgainButton));
    SetTextColor(views::Button::STATE_HOVERED,
                 cp->GetColor(kColorDialogDontAskAgainButtonHovered));
  }
};

BEGIN_METADATA(DontAskAgainButton, views::LabelButton)
END_METADATA

class CustomCheckbox : public views::Checkbox {
 public:
  METADATA_HEADER(CustomCheckbox);

  explicit CustomCheckbox(const std::u16string& label) : Checkbox(label) {
    SetFontList();
  }
  ~CustomCheckbox() override = default;
  CustomCheckbox(const CustomCheckbox&) = delete;
  CustomCheckbox& operator=(const CustomCheckbox&) = delete;

 private:
  void SetFontList() {
    gfx::FontList font_list;
    constexpr int kFontSize = 14;
    font_list.Derive(kFontSize - font_list.GetFontSize(),
                     font_list.GetFontStyle(), gfx::Font::Weight::NORMAL);
    label()->SetFontList(font_list);
  }
};

BEGIN_METADATA(CustomCheckbox, views::Checkbox)
END_METADATA

}  // namespace

BraveDefaultBrowserDialogView::BraveDefaultBrowserDialogView() {
  set_should_ignore_snapping(true);

  SetButtonLabel(ui::DIALOG_BUTTON_OK,
                 brave_l10n::GetLocalizedResourceUTF16String(
                     IDS_BRAVE_DEFAULT_BROWSER_DIALOG_OK_BUTTON_LABEL));
  SetButtonLabel(ui::DIALOG_BUTTON_CANCEL,
                 brave_l10n::GetLocalizedResourceUTF16String(
                     IDS_BRAVE_DEFAULT_BROWSER_DIALOG_CANCEL_BUTTON_LABEL));

  SetAcceptCallback(
      base::BindOnce(&BraveDefaultBrowserDialogView::OnAcceptButtonClicked,
                     base::Unretained(this)));
  SetCancelCallback(
      base::BindOnce(&BraveDefaultBrowserDialogView::OnCancelButtonClicked,
                     base::Unretained(this)));
  CreateChildViews();
}

BraveDefaultBrowserDialogView::~BraveDefaultBrowserDialogView() = default;

void BraveDefaultBrowserDialogView::CreateChildViews() {
  constexpr int kChildSpacing = 16;
  constexpr int kBottomPadding = 36;

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::TLBR(kPadding, kPadding, kBottomPadding, kPadding),
      kChildSpacing));

  // Use 15px font size for header text.
  int size_diff = 15 - views::Label::GetDefaultFontList().GetFontSize();
  views::Label::CustomFont header_font = {
      views::Label::GetDefaultFontList()
          .DeriveWithSizeDelta(size_diff)
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD)};
  header_label_ = AddChildView(std::make_unique<views::Label>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_BRAVE_DEFAULT_BROWSER_DIALOG_HEADER_TEXT),
      header_font));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  // Use 13px font size for contents text.
  size_diff = 13 - views::Label::GetDefaultFontList().GetFontSize();
  views::Label::CustomFont contents_font = {
      views::Label::GetDefaultFontList()
          .DeriveWithSizeDelta(size_diff)
          .DeriveWithWeight(gfx::Font::Weight::NORMAL)};
  contents_label_ = AddChildView(std::make_unique<views::Label>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_BRAVE_DEFAULT_BROWSER_DIALOG_CONTENTS_TEXT),
      contents_font));
  contents_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  contents_label_->SetMultiLine(true);
  contents_label_->SetMaximumWidth(350);

#if BUILDFLAG(IS_WIN)
  pin_shortcut_checkbox_ = AddChildView(std::make_unique<CustomCheckbox>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_FIRSTRUN_DLG_PIN_SHORTCUT_TEXT)));
  SetExtraView(std::make_unique<DontAskAgainButton>(
      views::Button::PressedCallback(base::BindRepeating(
          &BraveDefaultBrowserDialogView::OnDontAskAgainButtonPressed,
          base::Unretained(this)))));
#else
  dont_ask_again_checkbox_ = AddChildView(std::make_unique<CustomCheckbox>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_BRAVE_DEFAULT_BROWSER_DIALOG_DONT_ASK)));
#endif
}

ui::ModalType BraveDefaultBrowserDialogView::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

bool BraveDefaultBrowserDialogView::ShouldShowCloseButton() const {
  return false;
}

void BraveDefaultBrowserDialogView::OnWidgetInitialized() {
  SetButtonRowInsets(gfx::Insets::TLBR(0, kPadding, kPadding, kPadding));
}

void BraveDefaultBrowserDialogView::OnCancelButtonClicked() {
#if !BUILDFLAG(IS_WIN)
  g_browser_process->local_state()->SetBoolean(
      kDefaultBrowserPromptEnabled, !dont_ask_again_checkbox_->GetChecked());
#endif
}

void BraveDefaultBrowserDialogView::OnAcceptButtonClicked() {
  // The worker pointer is reference counted. While it is running, the
  // message loops of the FILE and UI thread will hold references to it
  // and it will be automatically freed once all its tasks have finished.
  base::MakeRefCounted<shell_integration::BraveDefaultBrowserWorker>()
#if BUILDFLAG(IS_WIN)
      ->StartSetAsDefault(base::BindOnce(
          [](bool pin_to_taskbar,
             shell_integration::DefaultWebClientState state) {
            if (state == shell_integration::DefaultWebClientState::IS_DEFAULT) {
              // Try to pin to taskbar when Brave is set as a default browser.
              shell_integration::win::PinToTaskbar();
            }
          },
          pin_shortcut_checkbox_->GetChecked()));
#else
      ->StartSetAsDefault(base::NullCallback());
#endif
}

#if BUILDFLAG(IS_WIN)
void BraveDefaultBrowserDialogView::OnDontAskAgainButtonPressed() {
  g_browser_process->local_state()->SetBoolean(kDefaultBrowserPromptEnabled,
                                               false);
  CancelDialog();
}
#endif

BEGIN_METADATA(BraveDefaultBrowserDialogView, views::DialogDelegateView)
END_METADATA
