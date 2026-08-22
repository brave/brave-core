// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/brave_news_page_action_controller.h"

#include "base/functional/bind.h"
#include "brave/browser/ui/views/brave_news/brave_news_bubble_controller.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/actions/actions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/views/bubble/bubble_anchor.h"
#include "ui/views/view.h"

namespace page_actions {

BraveNewsPageActionController::BraveNewsPageActionController(
    tabs::TabInterface& tab,
    page_actions::PageActionController& page_action_controller)
    : tab_(tab),
      page_action_controller_(
          static_cast<page_actions::PageActionControllerImpl&>(
              page_action_controller)) {
  Profile* const profile = tab_->GetBrowserWindowInterface()->GetProfile();
  should_show_.Init(
      brave_news::prefs::kShouldShowToolbarButton, profile->GetPrefs(),
      base::BindRepeating(&BraveNewsPageActionController::OnPrefChanged,
                          base::Unretained(this)));
  opted_in_.Init(
      brave_news::prefs::kBraveNewsOptedIn, profile->GetPrefs(),
      base::BindRepeating(&BraveNewsPageActionController::OnPrefChanged,
                          base::Unretained(this)));
  news_enabled_.Init(
      brave_news::prefs::kNewTabPageShowToday, profile->GetPrefs(),
      base::BindRepeating(&BraveNewsPageActionController::OnPrefChanged,
                          base::Unretained(this)));
}

BraveNewsPageActionController::~BraveNewsPageActionController() = default;

void BraveNewsPageActionController::Init() {
  did_activate_subscription_ = tab_->RegisterDidActivate(base::BindRepeating(
      [](BraveNewsPageActionController* self, tabs::TabInterface* tab) {
        self->AttachToTabHelper(tab->GetContents());
        self->UpdatePageAction(tab->GetContents());
      },
      base::Unretained(this)));
  will_discard_contents_subscription_ =
      tab_->RegisterWillDiscardContents(base::BindRepeating(
          [](BraveNewsPageActionController* self, tabs::TabInterface*,
             content::WebContents*, content::WebContents* new_contents) {
            // TabInterface::GetContents() still returns the outgoing contents
            // at this point, so the swapped in contents has to be taken from
            // the argument.
            self->AttachToTabHelper(new_contents);
            self->UpdatePageAction(new_contents);
          },
          base::Unretained(this)));
  AttachToTabHelper(tab_->GetContents());
  UpdatePageAction(tab_->GetContents());
}

void BraveNewsPageActionController::ExecuteAction(
    ToolbarButtonProvider* toolbar_button_provider,
    actions::ActionItem* item) {
  content::WebContents* const contents = tab_->GetContents();
  if (!contents || !toolbar_button_provider) {
    return;
  }

  views::View* const anchor_view =
      toolbar_button_provider->GetPageActionBubbleAnchor(kActionShowBraveNews)
          .GetIfView();
  if (!anchor_view || !anchor_view->GetWidget()) {
    return;
  }

  if (auto* controller =
          brave_news::BraveNewsBubbleController::CreateOrGetFromWebContents(
              contents)) {
    controller->ShowBubble(anchor_view);
  }
}

void BraveNewsPageActionController::OnAvailableFeedsChanged(
    const std::vector<GURL>& feeds) {
  UpdatePageAction(tab_->GetContents());
}

void BraveNewsPageActionController::WebContentsDestroyed() {
  page_feeds_observer_.Reset();
  Observe(nullptr);
}

void BraveNewsPageActionController::OnPrefChanged() {
  UpdatePageAction(tab_->GetContents());
}

void BraveNewsPageActionController::AttachToTabHelper(
    content::WebContents* contents) {
  BraveNewsTabHelper* const tab_helper =
      contents ? BraveNewsTabHelper::FromWebContents(contents) : nullptr;

  if (contents && tab_helper) {
    if (!page_feeds_observer_.IsObservingSource(tab_helper)) {
      page_feeds_observer_.Reset();
      page_feeds_observer_.Observe(tab_helper);
    }
    if (web_contents() != contents) {
      Observe(contents);
    }
  } else {
    if (web_contents()) {
      Observe(nullptr);
    }
    if (page_feeds_observer_.IsObserving()) {
      page_feeds_observer_.Reset();
    }
  }
}

void BraveNewsPageActionController::UpdatePageAction(
    content::WebContents* contents) {
  BraveNewsTabHelper* const tab_helper =
      contents ? BraveNewsTabHelper::FromWebContents(contents) : nullptr;

  if (!tab_helper || !should_show_.GetValue() || !news_enabled_.GetValue() ||
      !opted_in_.GetValue()) {
    page_action_controller_->Hide(kActionShowBraveNews);
    return;
  }

  const bool subscribed = tab_helper->IsSubscribed();
  const ui::ColorProvider& color_provider = contents->GetColorProvider();
  ui::ColorId color_id = kColorOmniboxActionIcon;
  if (subscribed) {
    color_id = nala::kColorIconInteractive;
  }
  page_action_controller_->OverrideImage(
      kActionShowBraveNews,
      ui::ImageModel::FromVectorIcon(kLeoRssIcon,
                                     color_provider.GetColor(color_id)));
  page_action_controller_->OverrideTooltip(
      kActionShowBraveNews,
      l10n_util::GetStringUTF16(IDS_BRAVE_NEWS_ACTION_VIEW_TOOLTIP));

  const bool has_feeds = !tab_helper->GetAvailableFeedUrls().empty();
  const bool bubble_showing =
      brave_news::BraveNewsBubbleController::CreateOrGetFromWebContents(
          contents)
          ->GetBubble() != nullptr;
  if (has_feeds || bubble_showing) {
    page_action_controller_->Show(kActionShowBraveNews);
  } else {
    page_action_controller_->Hide(kActionShowBraveNews);
  }
}

}  // namespace page_actions
