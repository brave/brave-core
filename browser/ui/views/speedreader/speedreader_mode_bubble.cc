/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/speedreader/speedreader_mode_bubble.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/browser/ui/views/speedreader/speedreader_bubble_util.h"
#include "brave/common/url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/common/referrer.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/window_open_disposition.h"
#include "ui/events/event.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/button/toggle_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace {

constexpr int kBubbleWidth = 324;  // width is 324 pixels

constexpr int kFontSizeSiteTitle = 14;  // site title font size

constexpr SkColor kColorButtonTrack = SkColorSetRGB(0xe1, 0xe2, 0xf6);

constexpr SkColor kColorButtonThumb = SkColorSetRGB(0x4c, 0x54, 0xd2);

}  // anonymous namespace

namespace speedreader {

SpeedreaderModeBubble::SpeedreaderModeBubble(views::View* anchor_view,
                                             SpeedreaderTabHelper* tab_helper)
    : LocationBarBubbleDelegateView(anchor_view, nullptr),
      tab_helper_(tab_helper) {
  SetButtons(ui::DialogButton::DIALOG_BUTTON_NONE);
}

void SpeedreaderModeBubble::Show() {
  ShowForReason(USER_GESTURE);
}

void SpeedreaderModeBubble::Hide() {
  if (tab_helper_) {
    tab_helper_->OnBubbleClosed();
    tab_helper_ = nullptr;
  }
  CloseBubble();
}

gfx::Size SpeedreaderModeBubble::CalculatePreferredSize() const {
  return gfx::Size(
      kBubbleWidth,
      LocationBarBubbleDelegateView::CalculatePreferredSize().height());
}

bool SpeedreaderModeBubble::ShouldShowCloseButton() const {
  return true;
}

void SpeedreaderModeBubble::WindowClosing() {
  if (tab_helper_) {
    tab_helper_->OnBubbleClosed();
    tab_helper_ = nullptr;
  }
}

void SpeedreaderModeBubble::Init() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(),
      kBoxLayoutChildSpacing));

  // Create sublayout for button and site title
  auto site_toggle_view = std::make_unique<views::View>();
  views::BoxLayout* site_toggle_layout =
      site_toggle_view->SetLayoutManager(std::make_unique<views::BoxLayout>());

  // Extract site title from webcontents, bolden it
  // fixme: for boldness we can do a style range on a label
  const auto host = tab_helper_->web_contents()->GetLastCommittedURL().host();
  DCHECK(!host.empty());
  auto site = base::ASCIIToUTF16(host);
  auto offset = site.length();
  site.append(kSpeedreaderSeparator);
  site.append(l10n_util::GetStringUTF16(IDS_PAGE_IS_DISTILLED));
  auto site_title_label = std::make_unique<views::StyledLabel>();
  site_title_label->SetText(site);
  site_title_label->SetLineHeight(kLineHeight);
  site_title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  views::StyledLabel::RangeStyleInfo style_title;
  style_title.custom_font = GetFont(
      kFontSizeSiteTitle, gfx::Font::Weight::SEMIBOLD);  // make host bold
  site_title_label->AddStyleRange(gfx::Range(0, offset), style_title);
  style_title.custom_font = GetFont(kFontSizeSiteTitle);  // disable bold
  site_title_label->AddStyleRange(gfx::Range(offset, site.length()),
                                  style_title);
  site_title_label_ =
      site_toggle_view->AddChildView(std::move(site_title_label));
  site_toggle_layout->SetFlexForView(
      site_title_label_,
      1);  // show the button and force text to wrap

  // float button right
  site_toggle_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kEnd);
  auto site_toggle_button =
      std::make_unique<views::ToggleButton>(base::BindRepeating(
          &SpeedreaderModeBubble::OnButtonPressed, base::Unretained(this)));
  // TODO(keur): We shoud be able to remove these once brave overrides
  // views::ToggleButton globally with our own theme
  site_toggle_button->SetThumbOnColor(kColorButtonThumb);
  site_toggle_button->SetTrackOnColor(kColorButtonTrack);
  site_toggle_button_ =
      site_toggle_view->AddChildView(std::move(site_toggle_button));

  AddChildView(std::move(site_toggle_view));

  auto site_toggle_explanation = BuildLabelWithEndingLink(
      l10n_util::GetStringUTF16(IDS_SPEEDREADER_DISABLE_THIS_SITE),
      l10n_util::GetStringUTF16(IDS_SETTINGS_TITLE),
      base::BindRepeating(&SpeedreaderModeBubble::OnLinkClicked,
                          base::Unretained(this)));
  site_toggle_explanation_ = AddChildView(std::move(site_toggle_explanation));
}

void SpeedreaderModeBubble::OnButtonPressed(const ui::Event& event) {
  // FIXME: Tie up this logic to the speedreader service. Disable just this
  // domain.
  NOTIMPLEMENTED();
}

void SpeedreaderModeBubble::OnLinkClicked(const ui::Event& event) {
  tab_helper_->web_contents()->OpenURL(
      content::OpenURLParams(GURL("chrome://settings"), content::Referrer(),
                             WindowOpenDisposition::NEW_FOREGROUND_TAB,
                             ui::PAGE_TRANSITION_LINK, false));
}

BEGIN_METADATA(SpeedreaderModeBubble, LocationBarBubbleDelegateView)
END_METADATA

}  // namespace speedreader
