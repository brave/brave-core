/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_H_

#include <vector>

#include "base/gtest_prod_util.h"
#include "brave/browser/ui/views/location_bar/brave_news_location_view.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveActionsContainer;
class BraveActionsContainerTest;
class PlaylistActionIconView;
class RewardsBrowserTest;
class SkPath;

#if BUILDFLAG(ENABLE_TOR)
class OnionLocationView;
#endif

#if BUILDFLAG(ENABLE_IPFS)
class IPFSLocationView;
#endif

namespace policy {
FORWARD_DECLARE_TEST(BraveRewardsPolicyTest, RewardsIconIsHidden);
}

// The purposes of this subclass are to:
// - Add the BraveActionsContainer to the location bar
class BraveLocationBarView : public LocationBarView {
 public:
  METADATA_HEADER(BraveLocationBarView);
  using LocationBarView::LocationBarView;

  BraveLocationBarView(const BraveLocationBarView&) = delete;
  BraveLocationBarView& operator=(const BraveLocationBarView&) = delete;

  void Init() override;
  void Update(content::WebContents* contents) override;
  void OnChanged() override;
  BraveActionsContainer* GetBraveActionsContainer() { return brave_actions_; }
#if BUILDFLAG(ENABLE_TOR)
  OnionLocationView* GetOnionLocationView() { return onion_location_view_; }
#endif

#if BUILDFLAG(ENABLE_IPFS)
  IPFSLocationView* GetIPFSLocationView() { return ipfs_location_view_; }
#endif
  // LocationBarView:
  std::vector<views::View*> GetTrailingViews() override;

  ui::ImageModel GetLocationIcon(LocationIconView::Delegate::IconFetchedCallback
                                     on_icon_fetched) const override;
  void OnOmniboxBlurred() override;

  // views::View:
  gfx::Size CalculatePreferredSize() const override;
  void OnThemeChanged() override;
  void ChildVisibilityChanged(views::View* child) override;

  int GetBorderRadius() const override;

  SkPath GetFocusRingHighlightPath() const;
  ContentSettingImageView* GetContentSettingsImageViewForTesting(size_t idx);
  bool ShouldShowIPFSLocationView() const;
  BraveActionsContainer* brave_actions_contatiner_view() {
    return brave_actions_;
  }

  void ShowPlaylistBubble();

 private:
  FRIEND_TEST_ALL_PREFIXES(policy::BraveRewardsPolicyTest, RewardsIconIsHidden);
  FRIEND_TEST_ALL_PREFIXES(PlaylistBrowserTest, AddItemsToList);
  friend class ::BraveActionsContainerTest;
  friend class ::RewardsBrowserTest;

  PlaylistActionIconView* GetPlaylistActionIconView();

  raw_ptr<BraveActionsContainer> brave_actions_ = nullptr;
  raw_ptr<BraveNewsLocationView> brave_news_location_view_ = nullptr;
#if BUILDFLAG(ENABLE_TOR)
  raw_ptr<OnionLocationView> onion_location_view_ = nullptr;
#endif
#if BUILDFLAG(ENABLE_IPFS)
  raw_ptr<IPFSLocationView> ipfs_location_view_ = nullptr;
#endif
};

#endif  // BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_H_
