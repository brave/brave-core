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
#include "brave/browser/ui/views/brave_news/brave_news_feed_item_view.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "include/core/SkColor.h"
#include "ui/accessibility/ax_enums.mojom-shared.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
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
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"

namespace {
SkColor kSubtitleColor = SkColorSetRGB(134, 142, 150);
}

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
                                      views::BubbleBorder::STANDARD_SHADOW),
      contents_(contents) {
  DCHECK(contents);

  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetAccessibleRole(ax::mojom::Role::kDialog);
  set_adjust_if_offscreen(true);

  this->SetProperty(views::kInternalPaddingKey, gfx::Insets::VH(16, 16));

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
  subtitle_label_->SetEnabledColor(kSubtitleColor);
  subtitle_label_->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);

  views::FlexLayout* const layout =
      SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetMainAxisAlignment(views::LayoutAlignment::kStart);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kStretch);
  layout->SetCollapseMargins(true);

  auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents);
  for (const auto& feed_item : tab_helper->GetAvailableFeeds()) {
    auto* child = AddChildView(
        std::make_unique<BraveNewsFeedItemView>(feed_item, contents));
    child->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(10, 0, 0, 0));
  }

  auto* dismiss_button = AddChildView(std::make_unique<views::MdTextButton>(
      base::BindRepeating(&BraveNewsBubbleView::DismissForever,
                          base::Unretained(this)),
      l10n_util::GetStringUTF16(IDS_BRAVE_NEWS_BUBBLE_DISMISS_FOREVER)));
  dismiss_button->SetProperty(views::kMarginsKey,
                              gfx::Insets::TLBR(10, 0, 0, 0));
  dismiss_button->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kPreferred));
}

BraveNewsBubbleView::~BraveNewsBubbleView() = default;

void BraveNewsBubbleView::DismissForever() {
  GetWidget()->Hide();
  auto* profile = Profile::FromBrowserContext(contents_->GetBrowserContext());
  profile->GetPrefs()->SetBoolean(brave_news::prefs::kShouldShowToolbarButton,
                                  false);
}

BEGIN_METADATA(BraveNewsBubbleView, views::BubbleDialogDelegateView)
END_METADATA
