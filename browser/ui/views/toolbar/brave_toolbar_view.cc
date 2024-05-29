/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/browser/ui/views/toolbar/bookmark_button.h"
#include "brave/browser/ui/views/toolbar/wallet_button.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/bookmarks/bookmark_bubble_sign_in_delegate.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bubble_view.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "ui/base/hit_test.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/window_open_disposition_utils.h"
#include "ui/events/event.h"
#include "ui/views/window/hit_test_utils.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/vpn_utils.h"
#include "brave/browser/ui/views/toolbar/brave_vpn_button.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(IS_LINUX)
#include "chrome/common/pref_names.h"
#endif

namespace {
constexpr int kLocationBarMaxWidth = 1080;

double GetLocationBarMarginHPercent(int toolbar_width) {
  double location_bar_margin_h_pc = 0.07;
  if (toolbar_width < 700)
    location_bar_margin_h_pc = 0;
  else if (toolbar_width < 850)
    location_bar_margin_h_pc = 0.03;
  else if (toolbar_width < 1000)
    location_bar_margin_h_pc = 0.05;
  return location_bar_margin_h_pc;
}

gfx::Insets CalcLocationBarMargin(int toolbar_width,
                                  int available_location_bar_width,
                                  int location_bar_min_width,
                                  int location_bar_x) {
  // Apply the target margin, adjusting for min and max width of LocationBar
  // Make sure any margin doesn't shrink the LocationBar beyond minimum width
  int location_bar_max_margin_h =
      (available_location_bar_width - location_bar_min_width) / 2;
  int location_bar_margin_h =
      std::min(static_cast<int>(toolbar_width *
                                GetLocationBarMarginHPercent(toolbar_width)),
               location_bar_max_margin_h);
  int location_bar_width =
      available_location_bar_width - (location_bar_margin_h * 2);
  // Allow the margin to expand so LocationBar is restrained to max width
  if (location_bar_width > kLocationBarMaxWidth) {
    location_bar_margin_h += (location_bar_width - kLocationBarMaxWidth) / 2;
    location_bar_width = kLocationBarMaxWidth;
  }

  // Center LocationBar as much as possible within Toolbar
  const int location_bar_toolbar_center_point =
      location_bar_x + location_bar_margin_h + (location_bar_width / 2);
  // Calculate offset - positive for move left and negative for move right
  int location_bar_center_offset =
      location_bar_toolbar_center_point - (toolbar_width / 2);
  // Can't shim more than we have space for, so restrict to margin size
  // or in the case of moving-right, 25% of the space since we want to avoid
  // touching browser actions where possible
  location_bar_center_offset =
      (location_bar_center_offset > 0)
          ? std::min(location_bar_margin_h, location_bar_center_offset)
          : std::max(static_cast<int>(-location_bar_margin_h * .25),
                     location_bar_center_offset);

  // // Apply offset to margin
  const int location_bar_margin_l =
      location_bar_margin_h - location_bar_center_offset;
  const int location_bar_margin_r =
      location_bar_margin_h + location_bar_center_offset;
  return gfx::Insets::TLBR(0, location_bar_margin_l, 0, location_bar_margin_r);
}

bool HasMultipleUserProfiles() {
  ProfileAttributesStorage* profile_storage =
      &g_browser_process->profile_manager()->GetProfileAttributesStorage();
  size_t profile_count = profile_storage->GetNumberOfProfiles();
  return (profile_count != 1);
}

bool IsAvatarButtonHideable(Profile* profile) {
  return !profile->IsIncognitoProfile() && !profile->IsGuestSession();
}

}  // namespace

BraveToolbarView::BraveToolbarView(Browser* browser, BrowserView* browser_view)
    : ToolbarView(browser, browser_view) {}

BraveToolbarView::~BraveToolbarView() = default;

void BraveToolbarView::Init() {
  ToolbarView::Init();

  // This will allow us to move this window by dragging toolbar.
  // See brave_non_client_hit_test_helper.h
  views::SetHitTestComponent(this, HTCAPTION);
  if (features::IsChromeRefresh2023()) {
    // Upstream has two more children |background_view_left_| and
    // |background_view_right_| behind the container view.
    DCHECK_EQ(3u, children().size());
  } else {
    DCHECK_EQ(1u, children().size());
  }
  views::SetHitTestComponent(children()[0], HTCAPTION);

  // For non-normal mode, we don't have to more.
  if (display_mode_ != DisplayMode::NORMAL) {
    brave_initialized_ = true;
    return;
  }

  Profile* profile = browser()->profile();

  // Track changes in profile count
  if (IsAvatarButtonHideable(profile)) {
    profile_observer_.Observe(
        &g_browser_process->profile_manager()->GetProfileAttributesStorage());
  }
  // track changes in bookmarks enabled setting
  edit_bookmarks_enabled_.Init(
      bookmarks::prefs::kEditBookmarksEnabled, profile->GetPrefs(),
      base::BindRepeating(&BraveToolbarView::OnEditBookmarksEnabledChanged,
                          base::Unretained(this)));
  show_bookmarks_button_.Init(
      kShowBookmarksButton, browser_->profile()->GetPrefs(),
      base::BindRepeating(&BraveToolbarView::OnShowBookmarksButtonChanged,
                          base::Unretained(this)));

  show_wallet_button_.Init(
      kShowWalletIconOnToolbar, browser_->profile()->GetPrefs(),
      base::BindRepeating(&BraveToolbarView::UpdateWalletButtonVisibility,
                          base::Unretained(this)));

  if (browser_->profile()->IsIncognitoProfile() &&
      !browser_->profile()->IsTor()) {
    wallet_private_window_enabled_.Init(
        kBraveWalletPrivateWindowsEnabled, browser_->profile()->GetPrefs(),
        base::BindRepeating(&BraveToolbarView::UpdateWalletButtonVisibility,
                            base::Unretained(this)));
  }

  // track changes in wide locationbar setting
  location_bar_is_wide_.Init(
      kLocationBarIsWide, profile->GetPrefs(),
      base::BindRepeating(&BraveToolbarView::OnLocationBarIsWideChanged,
                          base::Unretained(this)));

  if (tabs::utils::SupportsVerticalTabs(browser_)) {
    show_vertical_tabs_.Init(
        brave_tabs::kVerticalTabsEnabled,
        profile->GetOriginalProfile()->GetPrefs(),
        base::BindRepeating(&BraveToolbarView::UpdateHorizontalPadding,
                            base::Unretained(this)));
    show_title_bar_on_vertical_tabs_.Init(
        brave_tabs::kVerticalTabsShowTitleOnWindow,
        profile->GetOriginalProfile()->GetPrefs(),
        base::BindRepeating(&BraveToolbarView::UpdateHorizontalPadding,
                            base::Unretained(this)));
#if BUILDFLAG(IS_LINUX)
    use_custom_chrome_frame_.Init(
        prefs::kUseCustomChromeFrame, profile->GetOriginalProfile()->GetPrefs(),
        base::BindRepeating(&BraveToolbarView::UpdateHorizontalPadding,
                            base::Unretained(this)));
#endif  // BUILDFLAG(IS_LINUX)
  }

  const auto callback = [](Browser* browser, int command,
                           const ui::Event& event) {
    chrome::ExecuteCommandWithDisposition(
        browser, command, ui::DispositionFromEventFlags(event.flags()));
  };

  DCHECK(location_bar_);
  // Get ToolbarView's container_view as a parent of location_bar_ because
  // container_view's type in ToolbarView is internal to toolbar_view.cc.
  views::View* container_view = location_bar_->parent();
  DCHECK(container_view);
  bookmark_ = container_view->AddChildViewAt(
      std::make_unique<BraveBookmarkButton>(
          base::BindRepeating(callback, browser_, IDC_BOOKMARK_THIS_TAB)),
      *container_view->GetIndexOf(location_bar_));
  bookmark_->SetTriggerableEventFlags(ui::EF_LEFT_MOUSE_BUTTON |
                                      ui::EF_MIDDLE_MOUSE_BUTTON);
  bookmark_->UpdateImageAndText();

  wallet_ = container_view->AddChildViewAt(
      std::make_unique<WalletButton>(GetAppMenuButton(), profile),
      *container_view->GetIndexOf(GetAppMenuButton()) - 1);
  wallet_->SetTriggerableEventFlags(ui::EF_LEFT_MOUSE_BUTTON |
                                    ui::EF_MIDDLE_MOUSE_BUTTON);
  wallet_->UpdateImageAndText();

  UpdateWalletButtonVisibility();

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (brave_vpn::IsAllowedForContext(profile)) {
    brave_vpn_ = container_view->AddChildViewAt(
        std::make_unique<BraveVPNButton>(browser()),
        *container_view->GetIndexOf(GetAppMenuButton()) - 1);
    show_brave_vpn_button_.Init(
        brave_vpn::prefs::kBraveVPNShowButton, profile->GetPrefs(),
        base::BindRepeating(&BraveToolbarView::OnVPNButtonVisibilityChanged,
                            base::Unretained(this)));
    hide_brave_vpn_button_by_policy_.Init(
        brave_vpn::prefs::kManagedBraveVPNDisabled, profile->GetPrefs(),
        base::BindRepeating(&BraveToolbarView::OnVPNButtonVisibilityChanged,
                            base::Unretained(this)));
    brave_vpn_->SetVisible(IsBraveVPNButtonVisible());
  }
#endif

  // Make sure that avatar button should be located right before the app menu.
  if (auto* avatar = GetAvatarToolbarButton()) {
    container_view->ReorderChildView(
        avatar, *container_view->GetIndexOf(GetAppMenuButton()) - 1);
  }

  brave_initialized_ = true;
  UpdateHorizontalPadding();
}

#if BUILDFLAG(ENABLE_BRAVE_VPN)
bool BraveToolbarView::IsBraveVPNButtonVisible() const {
  return show_brave_vpn_button_.GetValue() &&
         !hide_brave_vpn_button_by_policy_.GetValue();
}
void BraveToolbarView::OnVPNButtonVisibilityChanged() {
  DCHECK(brave_vpn_);
  brave_vpn_->SetVisible(IsBraveVPNButtonVisible());
}
#endif

void BraveToolbarView::OnEditBookmarksEnabledChanged() {
  DCHECK_EQ(DisplayMode::NORMAL, display_mode_);
  Update(nullptr);
}

void BraveToolbarView::OnShowBookmarksButtonChanged() {
  if (!bookmark_)
    return;

  UpdateBookmarkVisibility();
}

void BraveToolbarView::OnLocationBarIsWideChanged() {
  DCHECK_EQ(DisplayMode::NORMAL, display_mode_);

  DeprecatedLayoutImmediately();
  SchedulePaint();
}

void BraveToolbarView::OnThemeChanged() {
  ToolbarView::OnThemeChanged();

  if (!brave_initialized_)
    return;

  if (display_mode_ == DisplayMode::NORMAL && bookmark_)
    bookmark_->UpdateImageAndText();
  if (display_mode_ == DisplayMode::NORMAL && wallet_)
    wallet_->UpdateImageAndText();
}

views::View* BraveToolbarView::GetAnchorView(
    std::optional<PageActionIconType> type) {
  if (features::IsSidePanelPinningEnabled()) {
    return ToolbarView::GetAnchorView(type);
  }
  return location_bar_;
}

void BraveToolbarView::OnProfileAdded(const base::FilePath& profile_path) {
  Update(nullptr);
}

void BraveToolbarView::OnProfileWasRemoved(const base::FilePath& profile_path,
                                           const std::u16string& profile_name) {
  Update(nullptr);
}

void BraveToolbarView::LoadImages() {
  ToolbarView::LoadImages();
  if (bookmark_)
    bookmark_->UpdateImageAndText();
  if (wallet_)
    wallet_->UpdateImageAndText();
}

void BraveToolbarView::Update(content::WebContents* tab) {
  ToolbarView::Update(tab);

  // Decide whether to show the bookmark button
  UpdateBookmarkVisibility();

  // Remove avatar menu if only a single user profile exists.
  // Always show if private / tor / guest window, as an indicator.
  auto* avatar_button = GetAvatarToolbarButton();
  if (avatar_button) {
    auto* profile = browser_->profile();
    const bool should_show_profile =
        !IsAvatarButtonHideable(profile) || HasMultipleUserProfiles();
    avatar_button->SetVisible(should_show_profile);
  }
}

void BraveToolbarView::UpdateBookmarkVisibility() {
  if (!bookmark_)
    return;

  DCHECK_EQ(DisplayMode::NORMAL, display_mode_);
  bookmark_->SetVisible(browser_defaults::bookmarks_enabled &&
                        edit_bookmarks_enabled_.GetValue() &&
                        show_bookmarks_button_.GetValue());
}

void BraveToolbarView::UpdateHorizontalPadding() {
  if (!brave_initialized_) {
    return;
  }

  // Get ToolbarView's container_view as a parent of location_bar_ because
  // container_view's type in ToolbarView is internal to toolbar_view.cc.
  DCHECK(location_bar_ && location_bar_->parent());
  views::View* container_view = location_bar_->parent();

  if (!tabs::utils::ShouldShowVerticalTabs(browser()) ||
      tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser())) {
    container_view->SetBorder(nullptr);
  } else {
    auto [leading, trailing] =
        tabs::utils::GetLeadingTrailingCaptionButtonWidth(
            browser_view_->frame());
    container_view->SetBorder(views::CreateEmptyBorder(
        gfx::Insets().set_left(leading).set_right(trailing)));
  }
}

void BraveToolbarView::ShowBookmarkBubble(const GURL& url,
                                          bool already_bookmarked) {
  // Show BookmarkBubble attached to Brave's bookmark button
  // or the location bar if there is no bookmark button
  // (i.e. in non-normal display mode).
  views::View* anchor_view = location_bar_;
  if (bookmark_ && bookmark_->GetVisible())
    anchor_view = bookmark_;

  std::unique_ptr<BubbleSignInPromoDelegate> delegate;
  delegate =
      std::make_unique<BookmarkBubbleSignInDelegate>(browser()->profile());
  BookmarkBubbleView::ShowBubble(anchor_view, GetWebContents(), bookmark_,
                                 std::move(delegate), browser_, url,
                                 already_bookmarked);
}

void BraveToolbarView::ViewHierarchyChanged(
    const views::ViewHierarchyChangedDetails& details) {
  ToolbarView::ViewHierarchyChanged(details);

  if (details.is_add && details.parent == children()[0]) {
    // Mark children of the container view as client area so that they are not
    // perceived as caption area. See brave_non_client_hit_test_helper.h
    views::SetHitTestComponent(details.child, HTCLIENT);
  }
}

void BraveToolbarView::Layout(PassKey) {
  LayoutSuperclass<ToolbarView>(this);

  if (!brave_initialized_)
    return;

  // ToolbarView::Layout() handles below modes. So just return.
  if (display_mode_ == DisplayMode::CUSTOM_TAB ||
      display_mode_ == DisplayMode::LOCATION) {
    return;
  }

  if (!location_bar_is_wide_.GetValue()) {
    ResetLocationBarBounds();
    ResetButtonBounds();
  }
}

void BraveToolbarView::ResetLocationBarBounds() {
  DCHECK_EQ(DisplayMode::NORMAL, display_mode_);

  // Calculate proper location bar's margin and set its bounds.
  const gfx::Insets margin = CalcLocationBarMargin(
      width(), location_bar_->width(), location_bar_->GetMinimumSize().width(),
      location_bar_->x());

  location_bar_->SetBounds(
      location_bar_->x() + margin.left(), location_bar_->y(),
      location_bar_->width() - margin.width(), location_bar_->height());
}

void BraveToolbarView::ResetButtonBounds() {
  DCHECK_EQ(DisplayMode::NORMAL, display_mode_);

  int button_right_margin = GetLayoutConstant(TOOLBAR_STANDARD_SPACING);

  if (bookmark_ && bookmark_->GetVisible()) {
    const int bookmark_width = bookmark_->GetPreferredSize().width();
    const int bookmark_x =
        location_bar_->x() - bookmark_width - button_right_margin;
    bookmark_->SetX(bookmark_x);
  }
}

void BraveToolbarView::UpdateWalletButtonVisibility() {
  Profile* profile = browser()->profile();
  if (brave_wallet::IsNativeWalletEnabled() &&
      brave_wallet::IsAllowedForContext(profile)) {
    // Hide all if user wants to hide.
    if (!show_wallet_button_.GetValue()) {
      wallet_->SetVisible(false);
      return;
    }

    if (!profile->IsIncognitoProfile()) {
      wallet_->SetVisible(true);
      return;
    }

    wallet_->SetVisible(wallet_private_window_enabled_.GetValue());
    return;
  }

  wallet_->SetVisible(false);
}

BEGIN_METADATA(BraveToolbarView)
END_METADATA
