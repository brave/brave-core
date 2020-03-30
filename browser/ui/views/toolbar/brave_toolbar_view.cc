/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/views/toolbar/bookmark_button.h"
#include "brave/browser/ui/views/toolbar/speedreader_button.h"
#include "brave/common/pref_names.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "brave/components/speedreader/speedreader_switches.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/bookmarks/bookmark_bubble_sign_in_delegate.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bubble_view.h"
#include "components/prefs/pref_service.h"
#include "components/bookmarks/common/bookmark_pref_names.h"

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
  int location_bar_max_margin_h = (
      available_location_bar_width - location_bar_min_width) / 2;
  int location_bar_margin_h =
      std::min(static_cast<int>(
                   toolbar_width * GetLocationBarMarginHPercent(toolbar_width)),
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
  location_bar_center_offset = (location_bar_center_offset > 0)
          ? std::min(location_bar_margin_h, location_bar_center_offset)
          : std::max(static_cast<int>(-location_bar_margin_h * .25),
                     location_bar_center_offset);

  // // Apply offset to margin
  const int location_bar_margin_l =
      location_bar_margin_h - location_bar_center_offset;
  const int location_bar_margin_r =
      location_bar_margin_h + location_bar_center_offset;
  return {0, location_bar_margin_l, 0, location_bar_margin_r};
}

bool HasMultipleUserProfiles() {
  ProfileAttributesStorage* profile_storage =
          &g_browser_process->profile_manager()->GetProfileAttributesStorage();
  size_t profile_count = profile_storage->GetNumberOfProfiles();
  return (profile_count != 1);
}

bool IsAvatarButtonHideable(Profile* profile) {
  return !brave::IsTorProfile(profile) &&
      !profile->IsIncognitoProfile() &&
      !profile->IsGuestSession();
}

}  // namespace

BraveToolbarView::BraveToolbarView(Browser* browser, BrowserView* browser_view)
    : ToolbarView(browser, browser_view),
      profile_observer_(this) { }

BraveToolbarView::~BraveToolbarView() {}

void BraveToolbarView::Init() {
  ToolbarView::Init();

  // For non-normal mode, we don't have to more.
  if (display_mode_ != DisplayMode::NORMAL) {
    brave_initialized_ = true;
    return;
  }

  Profile* profile = browser()->profile();

  // Track changes in profile count
  if (IsAvatarButtonHideable(profile)) {
    profile_observer_.Add(
        &g_browser_process->profile_manager()->GetProfileAttributesStorage());
  }
  // track changes in bookmarks enabled setting
  edit_bookmarks_enabled_.Init(
      bookmarks::prefs::kEditBookmarksEnabled, profile->GetPrefs(),
      base::Bind(&BraveToolbarView::OnEditBookmarksEnabledChanged,
                 base::Unretained(this)));
  // track changes in wide locationbar setting
  location_bar_is_wide_.Init(
      kLocationBarIsWide, profile->GetPrefs(),
      base::Bind(&BraveToolbarView::OnLocationBarIsWideChanged,
                 base::Unretained(this)));

  bookmark_ = new BookmarkButton(this);
  bookmark_->set_triggerable_event_flags(
      ui::EF_LEFT_MOUSE_BUTTON | ui::EF_MIDDLE_MOUSE_BUTTON);
  bookmark_->Init();

  // Speedreader.
  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();
  if (cmdline->HasSwitch(speedreader::kEnableSpeedreader)) {
    const bool speedreader_on = profile->GetPrefs()->GetBoolean(
        speedreader::kSpeedreaderEnabled);
    speedreader_ = new SpeedreaderButton(this, speedreader_on);
    speedreader_->set_triggerable_event_flags(
        ui::EF_LEFT_MOUSE_BUTTON | ui::EF_MIDDLE_MOUSE_BUTTON);
    speedreader_->Init();
  }

  DCHECK(location_bar_);
  AddChildViewAt(bookmark_, GetIndexOf(location_bar_));
  bookmark_->UpdateImage();
  if (speedreader_) {
    AddChildViewAt(speedreader_, GetIndexOf(location_bar_));
    speedreader_->UpdateImage();
  }

  brave_initialized_ = true;
}

void BraveToolbarView::OnEditBookmarksEnabledChanged() {
  DCHECK_EQ(DisplayMode::NORMAL, display_mode_);
  Update(nullptr);
}

void BraveToolbarView::OnLocationBarIsWideChanged() {
  DCHECK_EQ(DisplayMode::NORMAL, display_mode_);

  Layout();
  SchedulePaint();
}

void BraveToolbarView::OnThemeChanged() {
  ToolbarView::OnThemeChanged();

  if (!brave_initialized_)
    return;

  if (display_mode_ == DisplayMode::NORMAL && bookmark_)
    bookmark_->UpdateImage();
  if (display_mode_ == DisplayMode::NORMAL && speedreader_)
    speedreader_->UpdateImage();
}

void BraveToolbarView::OnProfileAdded(const base::FilePath& profile_path) {
  Update(nullptr);
}

void BraveToolbarView::OnProfileWasRemoved(
    const base::FilePath& profile_path,
    const base::string16& profile_name) {
  Update(nullptr);
}

void BraveToolbarView::LoadImages() {
  ToolbarView::LoadImages();
  if (bookmark_)
    bookmark_->UpdateImage();
  if (speedreader_)
    speedreader_->UpdateImage();
}

void BraveToolbarView::Update(content::WebContents* tab) {
  ToolbarView::Update(tab);
  // Decide whether to show the bookmark button
  if (bookmark_) {
    bookmark_->SetVisible(browser_defaults::bookmarks_enabled &&
                          edit_bookmarks_enabled_.GetValue());
  }
  if (speedreader_) {
    speedreader_->SetVisible(true);
  }
  // Remove avatar menu if only a single user profile exists.
  // Always show if private / tor / guest window, as an indicator.
  auto* avatar_button = GetAvatarToolbarButton();
  if (avatar_button) {
    auto* profile = browser_->profile();
    const bool should_show_profile =
      !IsAvatarButtonHideable(profile) ||
      HasMultipleUserProfiles();
    avatar_button->SetVisible(should_show_profile);
  }
}

void BraveToolbarView::ShowBookmarkBubble(
    const GURL& url,
    bool already_bookmarked,
    bookmarks::BookmarkBubbleObserver* observer) {
  // Show BookmarkBubble attached to Brave's bookmark button
  // or the location bar if there is no bookmark button
  // (i.e. in non-normal display mode).
  views::View* anchor_view = location_bar_;
  if (bookmark_ && bookmark_->GetVisible())
    anchor_view = bookmark_;

  std::unique_ptr<BubbleSyncPromoDelegate> delegate;
  delegate.reset(new BookmarkBubbleSignInDelegate(browser()));
  views::Widget* bubble_widget = BookmarkBubbleView::ShowBubble(
      anchor_view, bookmark_, gfx::Rect(), nullptr,
      observer, std::move(delegate), browser_->profile(),
      url, already_bookmarked);

  if (bubble_widget && bookmark_)
    bookmark_->OnBubbleWidgetCreated(bubble_widget);
}

void BraveToolbarView::Layout() {
  ToolbarView::Layout();

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
  const gfx::Insets margin =
      CalcLocationBarMargin(width(),
                            location_bar_->width(),
                            location_bar_->GetMinimumSize().width(),
      location_bar_->x());

  location_bar_->SetBounds(location_bar_->x() + margin.left(),
                           location_bar_->y(),
                           location_bar_->width() - margin.width(),
                           location_bar_->height());
}

void BraveToolbarView::ResetButtonBounds() {
  DCHECK_EQ(DisplayMode::NORMAL, display_mode_);

  int button_right_margin = GetLayoutConstant(TOOLBAR_STANDARD_SPACING);

  if (bookmark_ && bookmark_->GetVisible()) {
    const int bookmark_width = bookmark_->GetPreferredSize().width();
    const int bookmark_x =
        location_bar_->x() - bookmark_width - button_right_margin;
    bookmark_->SetX(bookmark_x);
    button_right_margin = bookmark_x;
  }

  if (speedreader_ && speedreader_->GetVisible()) {
    const int speedreader_width = speedreader_->GetPreferredSize().width();
    const int speedreader_x =
        button_right_margin - speedreader_width;
    speedreader_->SetX(speedreader_x);
  }
}
