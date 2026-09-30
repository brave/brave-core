// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/wayback_machine_infobar_delegate.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "brave/components/brave_wayback_machine/url_constants.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "url/gurl.h"

// Defined in wayback_machine_infobar_view.cc.
std::unique_ptr<infobars::InfoBar> CreateWaybackMachineInfoBar(
    std::unique_ptr<WaybackMachineInfoBarDelegate> delegate);

namespace {

constexpr int kIconSize = 24;

constinit base::TimeDelta g_countdown_tick_interval = base::Seconds(1);

}  // namespace

// static
void WaybackMachineInfoBarDelegate::Create(content::WebContents* contents) {
  auto* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(contents);
  auto* tab_helper = BraveWaybackMachineTabHelper::FromWebContents(contents);
  if (!infobar_manager || !tab_helper) {
    return;
  }
  CHECK_EQ(tab_helper->wayback_state(), WaybackState::kFound);
  infobar_manager->AddInfoBar(CreateWaybackMachineInfoBar(
      base::WrapUnique(new WaybackMachineInfoBarDelegate(*tab_helper))));
}

// static
base::AutoReset<base::TimeDelta>
WaybackMachineInfoBarDelegate::SetCountdownTickIntervalForTesting(
    base::TimeDelta interval) {
  return base::AutoReset<base::TimeDelta>(&g_countdown_tick_interval, interval);
}

WaybackMachineInfoBarDelegate::WaybackMachineInfoBarDelegate(
    BraveWaybackMachineTabHelper& tab_helper)
    : snapshot_time_(tab_helper.snapshot_time()) {
  wayback_state_changed_subscription_ =
      tab_helper.RegisterWaybackStateChangedCallback(base::BindRepeating(
          &WaybackMachineInfoBarDelegate::OnWaybackStateChanged,
          base::Unretained(this)));
  countdown_timer_.Start(FROM_HERE, g_countdown_tick_interval, this,
                         &WaybackMachineInfoBarDelegate::OnCountdownTick);
}

WaybackMachineInfoBarDelegate::~WaybackMachineInfoBarDelegate() = default;

void WaybackMachineInfoBarDelegate::SetCountdownChangedCallback(
    base::RepeatingClosure callback) {
  countdown_changed_callback_ = std::move(callback);
}

void WaybackMachineInfoBarDelegate::LoadNow() {
  countdown_timer_.Stop();
  auto* tab_helper = GetTabHelper();
  if (!tab_helper || tab_helper->wayback_state() != WaybackState::kFound) {
    Dismiss();
    return;
  }
  // Removes the infobar via OnWaybackStateChanged(), which may delete |this|.
  tab_helper->LoadWaybackURL();
}

void WaybackMachineInfoBarDelegate::Dismiss() {
  if (infobar() && infobar()->owner()) {
    infobar()->RemoveSelf();
  }
}

infobars::InfoBarDelegate::InfoBarIdentifier
WaybackMachineInfoBarDelegate::GetIdentifier() const {
  return BRAVE_WAYBACK_MACHINE_INFOBAR_DELEGATE;
}

ui::ImageModel WaybackMachineInfoBarDelegate::GetIcon() const {
  return ui::ImageModel::FromVectorIcon(kLeoInternetArchiveIcon,
                                        nala::kColorIconDefault, kIconSize);
}

std::u16string WaybackMachineInfoBarDelegate::GetLinkText() const {
  return l10n_util::GetStringUTF16(IDS_LEARN_MORE);
}

GURL WaybackMachineInfoBarDelegate::GetLinkURL() const {
  return GURL(kWaybackMachineLearnMoreURL);
}

bool WaybackMachineInfoBarDelegate::EqualsDelegate(
    infobars::InfoBarDelegate* delegate) const {
  return delegate->GetIdentifier() == GetIdentifier();
}

bool WaybackMachineInfoBarDelegate::IsCloseable() const {
  return false;
}

BraveWaybackMachineTabHelper* WaybackMachineInfoBarDelegate::GetTabHelper() {
  if (!infobar() || !infobar()->owner()) {
    return nullptr;
  }
  auto* contents =
      infobars::ContentInfoBarManager::WebContentsFromInfoBar(infobar());
  return contents ? BraveWaybackMachineTabHelper::FromWebContents(contents)
                  : nullptr;
}

void WaybackMachineInfoBarDelegate::OnCountdownTick() {
  --seconds_remaining_;
  if (seconds_remaining_ > 0) {
    if (countdown_changed_callback_) {
      countdown_changed_callback_.Run();
    }
    return;
  }
  LoadNow();
}

void WaybackMachineInfoBarDelegate::OnWaybackStateChanged(WaybackState state) {
  if (state != WaybackState::kFound) {
    Dismiss();
  }
}
