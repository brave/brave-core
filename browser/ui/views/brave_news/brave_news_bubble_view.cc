// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_news/brave_news_bubble_view.h"

#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/views/brave_news/brave_news_bubble_controller.h"
#include "brave/browser/ui/views/brave_news/brave_news_feed_item_view.h"
#include "brave/browser/ui/views/brave_news/brave_news_feeds_container_view.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/accessibility/ax_enums.mojom-shared.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/page_transition_types.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/background.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"

namespace {
constexpr SkColor kSubtitleColorLight = SkColorSetRGB(134, 142, 150);
constexpr SkColor kSubtitleColorDark = SkColorSetRGB(134, 142, 150);

constexpr SkColor kBackgroundColorLight = SkColorSetRGB(248, 249, 250);
constexpr SkColor kBackgroundColorDark = SkColorSetRGB(30, 32, 41);
}  // namespace

// static
base::WeakPtr<views::Widget> BraveNewsBubbleView::Show(
    views::View* anchor,
    content::WebContents* contents) {
  auto* widget = views::BubbleDialogDelegateView::CreateBubble(
      std::make_unique<BraveNewsBubbleView>(anchor, contents));
  widget->Show();
  return widget->GetWeakPtr();
}

BraveNewsBubbleView::BraveNewsBubbleView(views::View* action_view,
                                         content::WebContents* contents)
    : views::BubbleDialogDelegateView(action_view,
                                      views::BubbleBorder::TOP_RIGHT,
                                      views::BubbleBorder::STANDARD_SHADOW,
                                      /*autosize=*/true),
      contents_(contents) {
  DCHECK(contents);

  auto* controller =
      brave_news::BraveNewsBubbleController::FromWebContents(contents);
  CHECK(controller);
  controller_ = controller->AsWeakPtr();

  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetAccessibleWindowRole(ax::mojom::Role::kDialog);
  set_adjust_if_offscreen(true);

  SetProperty(views::kInternalPaddingKey, gfx::Insets::VH(16, 16));

  auto title_font_list = views::Label::GetDefaultFontList().DeriveWithWeight(
      gfx::Font::Weight::SEMIBOLD);
  title_font_list =
      title_font_list.DeriveWithSizeDelta(14 - title_font_list.GetFontSize());
  views::Label::CustomFont custom_font{title_font_list};
  title_label_ = AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_BRAVE_NEWS_BUBBLE_TITLE), custom_font));
  title_label_->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);

  auto subtitle_font_list = views::Label::GetDefaultFontList().DeriveWithWeight(
      gfx::Font::Weight::NORMAL);
  subtitle_font_list = subtitle_font_list.DeriveWithSizeDelta(
      12 - subtitle_font_list.GetFontSize());
  views::Label::CustomFont subtitle_custom_font{subtitle_font_list};

  subtitle_label_ = AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_BRAVE_NEWS_BUBBLE_SUBTITLE),
      subtitle_custom_font));
  subtitle_label_->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);
  subtitle_label_->SetProperty(views::kMarginsKey,
                               gfx::Insets::TLBR(0, 0, 16, 0));

  feeds_container_ =
      AddChildView(std::make_unique<BraveNewsFeedsContainerView>(contents));

  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical)
      .SetMainAxisAlignment(views::LayoutAlignment::kStart)
      .SetCrossAxisAlignment(views::LayoutAlignment::kStretch)
      .SetCollapseMargins(true);

  auto* manage_feeds_button =
      AddChildView(std::make_unique<views::MdTextButton>(
          base::BindRepeating(&BraveNewsBubbleView::OpenManageFeeds,
                              base::Unretained(this)),
          l10n_util::GetStringUTF16(IDS_BRAVE_NEWS_BUBBLE_MANAGE_FEEDS)));
  // Use tonal style here.
  manage_feeds_button->set_use_default_for_tonal(false);
  manage_feeds_button->SetStyle(ui::ButtonStyle::kTonal);
  manage_feeds_button->SetProperty(views::kMarginsKey,
                                   gfx::Insets::TLBR(10, 0, 0, 0));
  manage_feeds_button->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kPreferred));
  manage_feeds_button->SetProperty(views::kCrossAxisAlignmentKey,
                                   views::LayoutAlignment::kEnd);
  manage_feeds_button->SetIcon(&kLeoArrowRightIcon);
  manage_feeds_button->SetHorizontalAlignment(
      gfx::HorizontalAlignment::ALIGN_RIGHT);
}

BraveNewsBubbleView::~BraveNewsBubbleView() = default;

void BraveNewsBubbleView::OpenManageFeeds() {
  auto* browser = chrome::FindBrowserWithTab(contents_);
  browser->OpenURL(
      {GURL("brave://newtab/?openSettings=BraveNews"), content::Referrer(),
       WindowOpenDisposition::NEW_FOREGROUND_TAB, ui::PAGE_TRANSITION_LINK,
       false},
      /*navigation_handle_callback=*/{});
}

void BraveNewsBubbleView::OnWidgetDestroyed(views::Widget*) {
  if (!controller_) {
    return;
  }

  controller_->OnBubbleClosed();
}

void BraveNewsBubbleView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();

  auto is_dark = dark_mode::GetActiveBraveDarkModeType() ==
                 dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
  set_color(is_dark ? kBackgroundColorDark : kBackgroundColorLight);
  subtitle_label_->SetEnabledColor(is_dark ? kSubtitleColorDark
                                           : kSubtitleColorLight);
}

BEGIN_METADATA(BraveNewsBubbleView)
END_METADATA
