/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_default_browser_dialog_view.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/scoped_refptr.h"
#include "brave/browser/ui/browser_dialogs.h"
#include "brave/common/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/layout_provider.h"

namespace brave {

void ShowDefaultBrowserDialog(Browser* browser) {
  constrained_window::CreateBrowserModalDialogViews(
      new BraveDefaultBrowserDialogView(), browser->window()->GetNativeWindow())
      ->Show();
}

}  // namespace brave

namespace {

constexpr int kPadding = 24;

class NoSnappedBubbleFrameView : public views::BubbleFrameView {
 public:
  using BubbleFrameView::BubbleFrameView;
  ~NoSnappedBubbleFrameView() override = default;

  NoSnappedBubbleFrameView(const NoSnappedBubbleFrameView&) = delete;
  NoSnappedBubbleFrameView& operator=(const NoSnappedBubbleFrameView&) = delete;

 private:
  // views::BubbleFrarmeView overrides:
  // BubbleFrameView::GetFrameWidthForClientWidth() uses snapped dialog width
  // if dialog uses buttons. This width doesn't align with our design.
  int GetFrameWidthForClientWidth(int client_width) const override {
    // This doesn't use title bar. So, just using |client_width| is fine.
    return client_width;
  }
};

}  // namespace

BraveDefaultBrowserDialogView::BraveDefaultBrowserDialogView() {
  SetButtonLabel(ui::DIALOG_BUTTON_OK,
                 l10n_util::GetStringUTF16(
                     IDS_BRAVE_DEFAULT_BROWSER_DIALOG_OK_BUTTON_LABEL));
  SetButtonLabel(ui::DIALOG_BUTTON_CANCEL,
                 l10n_util::GetStringUTF16(
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
      gfx::Insets(kPadding, kPadding, kBottomPadding, kPadding),
      kChildSpacing));

  // Use 15px font size for header text.
  int size_diff = 15 - views::Label::GetDefaultFontList().GetFontSize();
  views::Label::CustomFont header_font = {
      views::Label::GetDefaultFontList()
          .DeriveWithSizeDelta(size_diff)
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD)};
  header_label_ = AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_BRAVE_DEFAULT_BROWSER_DIALOG_HEADER_TEXT),
      header_font));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  // Use 13px font size for contents text.
  size_diff = 13 - views::Label::GetDefaultFontList().GetFontSize();
  views::Label::CustomFont contents_font = {
      views::Label::GetDefaultFontList()
          .DeriveWithSizeDelta(size_diff)
          .DeriveWithWeight(gfx::Font::Weight::NORMAL)};
  contents_label_ = AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_BRAVE_DEFAULT_BROWSER_DIALOG_CONTENTS_TEXT),
      contents_font));
  contents_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  contents_label_->SetMultiLine(true);
  contents_label_->SetMaximumWidth(350);

  dont_ask_again_checkbox_ = AddChildView(std::make_unique<views::Checkbox>(
      l10n_util::GetStringUTF16(IDS_BRAVE_DEFAULT_BROWSER_DIALOG_DONT_ASK)));
}

std::unique_ptr<views::NonClientFrameView>
BraveDefaultBrowserDialogView::CreateNonClientFrameView(views::Widget* widget) {
  if (!use_custom_frame())
    return DialogDelegateView::CreateNonClientFrameView(widget);

  views::LayoutProvider* provider = views::LayoutProvider::Get();
  auto frame = std::make_unique<NoSnappedBubbleFrameView>(
      provider->GetInsetsMetric(views::INSETS_DIALOG_TITLE), gfx::Insets());

  const views::BubbleBorder::Shadow kShadow =
      views::BubbleBorder::DIALOG_SHADOW;
  std::unique_ptr<views::BubbleBorder> border =
      std::make_unique<views::BubbleBorder>(views::BubbleBorder::FLOAT, kShadow,
                                            gfx::kPlaceholderColor);
  border->set_use_theme_background_color(true);
  if (GetParams().round_corners)
    border->SetCornerRadius(GetCornerRadius());
  frame->SetFootnoteView(DisownFootnoteView());
  frame->SetBubbleBorder(std::move(border));
  return frame;
}

ui::ModalType BraveDefaultBrowserDialogView::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

bool BraveDefaultBrowserDialogView::ShouldShowCloseButton() const {
  return false;
}

void BraveDefaultBrowserDialogView::OnWidgetInitialized() {
  SetButtonRowInsets(gfx::Insets(0, kPadding, kPadding, kPadding));
}

void BraveDefaultBrowserDialogView::OnCancelButtonClicked() {
  g_browser_process->local_state()->SetBoolean(
      kDefaultBrowserPromptEnabled, !dont_ask_again_checkbox_->GetChecked());
}

void BraveDefaultBrowserDialogView::OnAcceptButtonClicked() {
  // The worker pointer is reference counted. While it is running, the
  // message loops of the FILE and UI thread will hold references to it
  // and it will be automatically freed once all its tasks have finished.
  base::MakeRefCounted<shell_integration::DefaultBrowserWorker>()
      ->StartSetAsDefault(base::NullCallback());
}
