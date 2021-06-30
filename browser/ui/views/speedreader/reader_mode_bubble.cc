/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/speedreader/reader_mode_bubble.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/notreached.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/browser/ui/views/speedreader/speedreader_bubble_util.h"
#include "brave/common/url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/common/referrer.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/window_open_disposition.h"
#include "ui/events/event.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace {

constexpr int kBubbleWidth = 264;  // width is 264 pixels

constexpr int kFontHeading = 16;  // heading text font size

constexpr int kFontSizeButton = 13;  // button text font size

}  // anonymous namespace

namespace speedreader {

// Material Design button, overriding the font list in the LabelButton.
class ReaderButton : public views::MdTextButton {
 public:
  explicit ReaderButton(PressedCallback callback = PressedCallback(),
                        const std::u16string& text = std::u16string(),
                        int button_context = views::style::CONTEXT_BUTTON_MD)
      : views::MdTextButton(callback, text, button_context) {
    label()->SetFontList(GetFont(kFontSizeButton, gfx::Font::Weight::SEMIBOLD));
  }

  void SetEnabledColor(SkColor color) { label()->SetEnabledColor(color); }
  ~ReaderButton() override = default;
};

ReaderModeBubble::ReaderModeBubble(views::View* anchor_view,
                                   SpeedreaderTabHelper* tab_helper)
    : LocationBarBubbleDelegateView(anchor_view, nullptr),
      tab_helper_(tab_helper) {
  SetButtons(ui::DialogButton::DIALOG_BUTTON_NONE);
}

void ReaderModeBubble::Show() {
  ShowForReason(USER_GESTURE);
}

void ReaderModeBubble::Hide() {
  if (tab_helper_) {
    tab_helper_->OnBubbleClosed();
    tab_helper_ = nullptr;
  }
  CloseBubble();
}

gfx::Size ReaderModeBubble::CalculatePreferredSize() const {
  return gfx::Size(
      kBubbleWidth,
      LocationBarBubbleDelegateView::CalculatePreferredSize().height());
}

bool ReaderModeBubble::ShouldShowCloseButton() const {
  return true;
}

void ReaderModeBubble::WindowClosing() {
  if (tab_helper_) {
    tab_helper_->OnBubbleClosed();
    tab_helper_ = nullptr;
  }
}

void ReaderModeBubble::Init() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(),
      kBoxLayoutChildSpacing));

  // Heading
  auto heading_label = std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_SPEEDREADER_ASK_ENABLE));
  heading_label->SetMultiLine(true);
  heading_label->SetFontList(
      GetFont(kFontHeading, gfx::Font::Weight::SEMIBOLD));
  heading_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  heading_label_ = AddChildView(std::move(heading_label));

  // Explanation of Speedreader features
  auto global_toggle_label = BuildLabelWithEndingLink(
      l10n_util::GetStringUTF16(IDS_SPEEDREADER_EXPLANATION),
      l10n_util::GetStringUTF16(IDS_LEARN_MORE),
      base::BindRepeating(&ReaderModeBubble::OnLinkClicked,
                          base::Unretained(this)));
  global_toggle_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  global_toggle_label->SetLineHeight(kLineHeight);
  global_toggle_label_ = AddChildView(std::move(global_toggle_label));

  // Enable Speedreader button
  auto enable_speedreader_button = std::make_unique<ReaderButton>(
      base::BindRepeating(&ReaderModeBubble::OnButtonPressed,
                          base::Unretained(this)),
      l10n_util::GetStringUTF16(IDS_SPEEDREADER_ENABLE_BUTTON));
  enable_speedreader_button_ =
      AddChildView(std::move(enable_speedreader_button));
}

void ReaderModeBubble::OnButtonPressed(const ui::Event& event) {
  // FIXME: Tie up this logic to the speedreader service. Enable Speedreader
  // globally.
  NOTIMPLEMENTED();
}

void ReaderModeBubble::OnLinkClicked(const ui::Event& event) {
  tab_helper_->web_contents()->OpenURL(content::OpenURLParams(
      GURL(kSpeedreaderLearnMoreUrl), content::Referrer(),
      WindowOpenDisposition::NEW_FOREGROUND_TAB, ui::PAGE_TRANSITION_LINK,
      false));
}

BEGIN_METADATA(ReaderModeBubble, LocationBarBubbleDelegateView)
END_METADATA

}  // namespace speedreader
