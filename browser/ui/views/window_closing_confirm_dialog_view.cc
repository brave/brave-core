/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/window_closing_confirm_dialog_view.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/constants/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/grit/branded_strings.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_delegate.h"

namespace {

base::RepeatingCallback<void(views::DialogDelegateView*)>&
GetCreationCallbackForTesting() {
  static base::NoDestructor<
      base::RepeatingCallback<void(views::DialogDelegateView*)>>
      callback;
  return *callback;
}

gfx::FontList GetFont(int font_size, gfx::Font::Weight weight) {
  gfx::FontList font_list;
  return font_list.DeriveWithSizeDelta(font_size - font_list.GetFontSize())
      .DeriveWithWeight(weight);
}

}  // namespace

// Subclass for custom font.
class DontAskAgainCheckbox : public views::Checkbox {
  METADATA_HEADER(DontAskAgainCheckbox, views::Checkbox)
 public:

  using views::Checkbox::Checkbox;
  ~DontAskAgainCheckbox() override = default;
  DontAskAgainCheckbox(const DontAskAgainCheckbox&) = delete;
  DontAskAgainCheckbox& operator=(const DontAskAgainCheckbox&) = delete;

  void SetFontList(const gfx::FontList& font_list) {
    label()->SetFontList(font_list);
  }
};

BEGIN_METADATA(DontAskAgainCheckbox)
END_METADATA

// static
void WindowClosingConfirmDialogView::Show(
    Browser* browser,
    base::OnceCallback<void(bool)> response_callback) {
  // The dialog eats mouse events which results in the close button
  // getting stuck in the hover state. Reset the window controls to
  // prevent this.
  BrowserView::GetBrowserViewForBrowser(browser)
      ->GetWidget()
      ->non_client_view()
      ->ResetWindowControls();

  auto* delegate =
      new WindowClosingConfirmDialogView(browser, std::move(response_callback));
  constrained_window::CreateBrowserModalDialogViews(
      delegate, browser->window()->GetNativeWindow())
      ->Show();

  if (GetCreationCallbackForTesting())
    GetCreationCallbackForTesting().Run(delegate);
}

// static
void WindowClosingConfirmDialogView::SetCreationCallbackForTesting(
    base::RepeatingCallback<void(views::DialogDelegateView*)>
        creation_callback) {
  GetCreationCallbackForTesting() = std::move(creation_callback);
}

WindowClosingConfirmDialogView::WindowClosingConfirmDialogView(
    Browser* browser,
    base::OnceCallback<void(bool)> response_callback)
    : browser_(browser),
      response_callback_(std::move(response_callback)),
      prefs_(browser->profile()->GetOriginalProfile()->GetPrefs()) {
  set_should_ignore_snapping(true);
  SetButtonLabel(ui::mojom::DialogButton::kOk,
                 l10n_util::GetStringUTF16(
                     IDS_WINDOW_CLOSING_CONFIRM_DLG_OK_BUTTON_LABEL));
  SetButtonLabel(ui::mojom::DialogButton::kCancel,
                 l10n_util::GetStringUTF16(
                     IDS_WINDOW_CLOSING_CONFIRM_DLG_CANCEL_BUTTON_LABEL));
  RegisterWindowClosingCallback(base::BindOnce(
      &WindowClosingConfirmDialogView::OnClosing, base::Unretained(this)));
  SetAcceptCallback(base::BindOnce(&WindowClosingConfirmDialogView::OnAccept,
                                   base::Unretained(this)));
  SetCancelCallback(base::BindOnce(&WindowClosingConfirmDialogView::OnCancel,
                                   base::Unretained(this)));

  constexpr int kChildSpacing = 16;
  constexpr int kPadding = 24;
  constexpr int kTopPadding = 32;
  constexpr int kBottomPadding = 26;

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::TLBR(kTopPadding, kPadding, kBottomPadding, kPadding),
      kChildSpacing));

  constexpr int kHeaderFontSize = 15;
  views::Label::CustomFont header_font = {
      GetFont(kHeaderFontSize, gfx::Font::Weight::SEMIBOLD)};
  auto* header_label = AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_WINDOW_CLOSING_CONFIRM_DLG_HEADER_LABEL),
      header_font));
  header_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  const int tab_count = browser_->tab_strip_model()->count();
  const std::u16string tab_count_part = l10n_util::GetStringFUTF16Int(
      IDS_WINDOW_CLOSING_CONFIRM_DLG_CONTENTS_LABEL_TAB_NUM_PART, tab_count);
  size_t offset;

  const std::u16string contents_text =
      l10n_util::GetStringFUTF16(IDS_WINDOW_CLOSING_CONFIRM_DLG_CONTENTS_LABEL,
                                 base::NumberToString16(tab_count), &offset);

  auto* contents_label = AddChildView(std::make_unique<views::StyledLabel>());
  contents_label->SetText(contents_text);

  constexpr int kContentsFontSize = 14;
  views::StyledLabel::RangeStyleInfo tab_count_style;
  tab_count_style.custom_font =
      GetFont(kContentsFontSize, gfx::Font::Weight::SEMIBOLD);
  contents_label->AddStyleRange(
      gfx::Range(offset, offset + tab_count_part.length()), tab_count_style);

  views::StyledLabel::RangeStyleInfo default_style;
  default_style.custom_font =
      GetFont(kContentsFontSize, gfx::Font::Weight::NORMAL);
  contents_label->AddStyleRange(
      gfx::Range(offset + tab_count_part.length(), contents_text.length()),
      default_style);
  if (offset != 0)
    contents_label->AddStyleRange(gfx::Range(0, offset), default_style);
  constexpr int kMaxWidth = 389;
  contents_label->SizeToFit(kMaxWidth);
  contents_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  dont_ask_again_checkbox_ = AddChildView(
      std::make_unique<DontAskAgainCheckbox>(l10n_util::GetStringUTF16(
          IDS_WINDOW_CLOSING_CONFIRM_DLG_DONT_ASK_AGAIN_LABEL)));
  dont_ask_again_checkbox_->SetFontList(
      GetFont(kContentsFontSize, gfx::Font::Weight::NORMAL));
}

WindowClosingConfirmDialogView::~WindowClosingConfirmDialogView() = default;

ui::mojom::ModalType WindowClosingConfirmDialogView::GetModalType() const {
  return ui::mojom::ModalType::kWindow;
}

bool WindowClosingConfirmDialogView::ShouldShowCloseButton() const {
  return false;
}

bool WindowClosingConfirmDialogView::ShouldShowWindowTitle() const {
  return false;
}

void WindowClosingConfirmDialogView::OnAccept() {
  close_window_ = true;
}

void WindowClosingConfirmDialogView::OnCancel() {
  close_window_ = false;
}

void WindowClosingConfirmDialogView::OnClosing() {
  prefs_->SetBoolean(kEnableWindowClosingConfirm,
                     !dont_ask_again_checkbox_->GetChecked());
  // Run callback here instead of by OnAccept() or OnCancel().
  // This dialog is modal and this callback could launch another modal dialog.
  // On macOS, new modal dialog seems not launched when this callback is called
  // more earlier than this closing callback.
  std::move(response_callback_).Run(close_window_);
}

BEGIN_METADATA(WindowClosingConfirmDialogView)
END_METADATA
