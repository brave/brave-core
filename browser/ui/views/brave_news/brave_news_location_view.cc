// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_news/brave_news_location_view.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "brave/browser/ui/views/brave_news/brave_news_bubble_controller.h"
#include "brave/browser/ui/views/brave_news/brave_news_bubble_view.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
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

  should_show_.Init(brave_news::prefs::kShouldShowToolbarButton,
                    profile->GetPrefs(),
                    base::BindRepeating(&BraveNewsLocationView::UpdateImpl,
                                        base::Unretained(this)));
  opted_in_.Init(brave_news::prefs::kBraveNewsOptedIn, profile->GetPrefs(),
                 base::BindRepeating(&BraveNewsLocationView::UpdateImpl,
                                     base::Unretained(this)));
  news_enabled_.Init(brave_news::prefs::kNewTabPageShowToday,
                     profile->GetPrefs(),
                     base::BindRepeating(&BraveNewsLocationView::UpdateImpl,
                                         base::Unretained(this)));

  Update();
}

BraveNewsLocationView::~BraveNewsLocationView() = default;

void BraveNewsLocationView::UpdateImpl() {
  auto* contents = GetWebContents();
  BraveNewsTabHelper* tab_helper =
      contents ? BraveNewsTabHelper::FromWebContents(contents) : nullptr;

  // When the active tab changes, subscribe to notification when
  // it has found a feed.
  if (contents && tab_helper) {
    // Observe BraveNewsTabHelper for feed changes
    if (!page_feeds_observer_.IsObservingSource(tab_helper)) {
      page_feeds_observer_.Reset();
      page_feeds_observer_.Observe(tab_helper);
    }
    // Observe WebContentsObserver for WebContentsDestroyed
    if (web_contents() != contents) {
      Observe(contents);
    }
  } else {
    // Unobserve WebContentsObserver
    if (web_contents()) {
      Observe(nullptr);
    }
    // Unobserve BraveNewsTabHelper
    if (page_feeds_observer_.IsObserving()) {
      page_feeds_observer_.Reset();
    }
  }

  // Don't show the icon if preferences don't allow
  if (!tab_helper || !should_show_.GetValue() || !news_enabled_.GetValue() ||
      !opted_in_.GetValue()) {
    SetVisible(false);
    return;
  }

  // Verify observing BraveNewsTabHelper
  DCHECK(page_feeds_observer_.IsObservingSource(tab_helper));
  // Verify observing for WebContentsDestroyed
  DCHECK(web_contents());

  // Icon color changes if any feeds are being followed
  UpdateIconColor(tab_helper->IsSubscribed());

  // Don't show icon if there are no feeds
  const bool has_feeds = !tab_helper->GetAvailableFeedUrls().empty();
  const bool is_visible = has_feeds || IsBubbleShowing();
  SetVisible(is_visible);
}

void BraveNewsLocationView::WebContentsDestroyed() {
  page_feeds_observer_.Reset();
  Observe(nullptr);
}

const gfx::VectorIcon& BraveNewsLocationView::GetVectorIcon() const {
  return kLeoRssIcon;
}

std::u16string BraveNewsLocationView::GetTextForTooltipAndAccessibleName()
    const {
  return l10n_util::GetStringUTF16(IDS_BRAVE_NEWS_ACTION_VIEW_TOOLTIP);
}

bool BraveNewsLocationView::ShouldShowLabel() const {
  return false;
}

void BraveNewsLocationView::OnAvailableFeedsChanged(
    const std::vector<GURL>& feeds) {
  Update();
}

void BraveNewsLocationView::OnThemeChanged() {
  bool subscribed = false;
  if (auto* contents = GetWebContents()) {
    subscribed = BraveNewsTabHelper::FromWebContents(contents)->IsSubscribed();
  }
  UpdateIconColor(subscribed);
  PageActionIconView::OnThemeChanged();
}

void BraveNewsLocationView::OnExecuting(
    PageActionIconView::ExecuteSource execute_source) {
  ShowBraveNewsBubble();
}

void BraveNewsLocationView::UpdateIconColor(bool subscribed) {
  SkColor icon_color;
  if (subscribed) {
    auto is_dark = GetNativeTheme()->GetPreferredColorScheme() ==
                   ui::NativeTheme::PreferredColorScheme::kDark;
    icon_color = is_dark ? kSubscribedDarkColor : kSubscribedLightColor;
  } else {
    icon_color = color_utils::DeriveDefaultIconColor(GetCurrentTextColor());
  }
  SetIconColor(icon_color);
}

brave_news::BraveNewsBubbleController* BraveNewsLocationView::GetController()
    const {
  auto* web_contents = GetWebContents();
  return web_contents ? brave_news::BraveNewsBubbleController::
                            CreateOrGetFromWebContents(web_contents)
                      : nullptr;
}

views::BubbleDialogDelegate* BraveNewsLocationView::GetBubble() const {
  auto* controller = GetController();
  return controller ? controller->GetBubble() : nullptr;
}

void BraveNewsLocationView::ShowBraveNewsBubble() {
  if (auto* controller = GetController()) {
    controller->ShowBubble(AsWeakPtr());
  }
}

base::WeakPtr<BraveNewsLocationView> BraveNewsLocationView::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

BEGIN_METADATA(BraveNewsLocationView)
END_METADATA
