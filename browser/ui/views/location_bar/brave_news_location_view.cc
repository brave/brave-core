// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/location_bar/brave_news_location_view.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/functional/callback_forward.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "brave/browser/ui/views/brave_news/brave_news_bubble_view.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

namespace {

constexpr SkColor kSubscribedLightColor = SkColorSetRGB(76, 84, 210);
constexpr SkColor kSubscribedDarkColor = SkColorSetRGB(115, 122, 222);

}  // namespace

BraveNewsLocationView::BraveNewsLocationView(
    Profile* profile,
    IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
    PageActionIconView::Delegate* page_action_icon_delegate)
    : PageActionIconView(/*command_updater=*/nullptr,
                         /*command_id=*/0,
                         icon_label_bubble_delegate,
                         page_action_icon_delegate,
                         "BraveNewsFollow") {
  SetLabel(l10n_util::GetStringUTF16(IDS_BRAVE_NEWS_ACTION_VIEW_TOOLTIP));

  last_contents_ = GetWebContents();
  if (last_contents_) {
    BraveNewsTabHelper::FromWebContents(last_contents_)->AddObserver(this);
  }

  should_show_.Init(brave_news::prefs::kShouldShowToolbarButton,
                    profile->GetPrefs(),
                    base::BindRepeating(&BraveNewsLocationView::UpdateImpl,
                                        base::Unretained(this)));
  opted_in_.Init(brave_news::prefs::kBraveTodayOptedIn, profile->GetPrefs(),
                 base::BindRepeating(&BraveNewsLocationView::UpdateImpl,
                                     base::Unretained(this)));
  news_enabled_.Init(brave_news::prefs::kNewTabPageShowToday,
                     profile->GetPrefs(),
                     base::BindRepeating(&BraveNewsLocationView::UpdateImpl,
                                         base::Unretained(this)));

  Update();
}

BraveNewsLocationView::~BraveNewsLocationView() = default;

views::BubbleDialogDelegate* BraveNewsLocationView::GetBubble() const {
  return bubble_view_;
}

void BraveNewsLocationView::UpdateImpl() {
  auto* contents = GetWebContents();
  BraveNewsTabHelper* tab_helper =
      contents ? BraveNewsTabHelper::FromWebContents(contents) : nullptr;

  // When the active tab changes, subscribe to notification when
  // it has found a feed, and update display with current state.
  if (contents != last_contents_) {
    // Unobserve old Tab
    if (last_contents_) {
      BraveNewsTabHelper::FromWebContents(last_contents_)->RemoveObserver(this);
    }
    last_contents_ = contents;
    // Observe new Tab
    if (contents) {
      tab_helper->AddObserver(this);
    }
  }

  // Don't show the icon if preferences don't allow
  if (!tab_helper || !should_show_.GetValue() || !news_enabled_.GetValue() ||
      !opted_in_.GetValue()) {
    SetVisible(false);
    return;
  }

  // Icon color changes if any feeds are being followed
  bool subscribed = false;
  subscribed = tab_helper->IsSubscribed();
  SetIconColor(GetIconColor(subscribed));

  // Don't show icon if there are no feeds
  const bool has_feeds = !tab_helper->GetAvailableFeeds().empty();
  const bool is_visible = has_feeds || IsBubbleShowing();
  SetVisible(is_visible);
}

const gfx::VectorIcon& BraveNewsLocationView::GetVectorIcon() const {
  return kBraveNewsSubscribeIcon;
}

std::u16string BraveNewsLocationView::GetTextForTooltipAndAccessibleName()
    const {
  return l10n_util::GetStringUTF16(IDS_BRAVE_NEWS_ACTION_VIEW_TOOLTIP);
}

bool BraveNewsLocationView::ShouldShowLabel() const {
  return false;
}

void BraveNewsLocationView::OnAvailableFeedsChanged(
    const std::vector<BraveNewsTabHelper::FeedDetails>& feeds) {
  Update();
}

void BraveNewsLocationView::OnThemeChanged() {
  views::LabelButton::OnThemeChanged();
  Update();
}

void BraveNewsLocationView::OnExecuting(
    PageActionIconView::ExecuteSource execute_source) {
  // If the bubble is already open, do nothing.
  if (IsBubbleShowing()) {
    return;
  }

  auto* contents = GetWebContents();
  if (!contents) {
    return;
  }

  bubble_view_ = new BraveNewsBubbleView(this, contents);
  bubble_view_->SetCloseCallback(base::BindOnce(
      &BraveNewsLocationView::OnBubbleClosed, base::Unretained(this)));
  auto* bubble_widget =
      views::BubbleDialogDelegateView::CreateBubble(bubble_view_);
  bubble_widget->Show();
}

void BraveNewsLocationView::OnBubbleClosed() {
  bubble_view_ = nullptr;
}

SkColor BraveNewsLocationView::GetIconColor(bool subscribed) const {
  if (!subscribed)
    return color_utils::DeriveDefaultIconColor(GetCurrentTextColor());

  auto is_dark = GetNativeTheme()->GetPreferredColorScheme() ==
                 ui::NativeTheme::PreferredColorScheme::kDark;
  return is_dark ? kSubscribedDarkColor : kSubscribedLightColor;
}
