/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_TOOLBAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_TOOLBAR_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "components/prefs/pref_member.h"
#include "ui/base/metadata/metadata_header_macros.h"

class AIChatButton;

#if BUILDFLAG(ENABLE_BRAVE_VPN)
class BraveVPNButton;
#endif

class BraveBookmarkButton;
class SidePanelButton;
class WalletButton;

class BraveToolbarView : public ToolbarView,
                         public ProfileAttributesStorage::Observer {
  METADATA_HEADER(BraveToolbarView, ToolbarView)
 public:
  explicit BraveToolbarView(Browser* browser, BrowserView* browser_view);
  ~BraveToolbarView() override;

  BraveBookmarkButton* bookmark_button() const { return bookmark_; }
  WalletButton* wallet_button() const { return wallet_; }
  SidePanelButton* side_panel_button() const { return side_panel_; }
  AIChatButton* ai_chat_button() const { return ai_chat_button_; }

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  BraveVPNButton* brave_vpn_button() const { return brave_vpn_; }
  bool IsBraveVPNButtonVisible() const;
  void OnVPNButtonVisibilityChanged();
#endif

  void UpdateHorizontalPadding();

  void Init() override;
  void Layout(PassKey) override;
  void Update(content::WebContents* tab) override;
  void OnThemeChanged() override;
  void OnEditBookmarksEnabledChanged();
  void OnLocationBarIsWideChanged();
  void OnShowBookmarksButtonChanged();
  void ShowBookmarkBubble(const GURL& url, bool already_bookmarked) override;
  void ViewHierarchyChanged(
      const views::ViewHierarchyChangedDetails& details) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveToolbarViewTest, ToolbarDividerNotShownTest);

  void LoadImages() override;
  void ResetLocationBarBounds();
  void ResetButtonBounds();
  void UpdateBookmarkVisibility();

  // ToolbarButtonProvider:
  views::View* GetAnchorView(std::optional<PageActionIconType> type) override;

  // ProfileAttributesStorage::Observer:
  void OnProfileAdded(const base::FilePath& profile_path) override;
  void OnProfileWasRemoved(const base::FilePath& profile_path,
                           const std::u16string& profile_name) override;

  void UpdateAIChatButtonVisibility();
  void UpdateWalletButtonVisibility();

  views::View* toolbar_divider_for_testing() { return toolbar_divider_; }

  raw_ptr<BraveBookmarkButton> bookmark_ = nullptr;
  // Tracks the preference to determine whether bookmark editing is allowed.
  BooleanPrefMember edit_bookmarks_enabled_;

  raw_ptr<WalletButton> wallet_ = nullptr;
  raw_ptr<SidePanelButton> side_panel_ = nullptr;

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  raw_ptr<BraveVPNButton> brave_vpn_ = nullptr;
  BooleanPrefMember show_brave_vpn_button_;
  BooleanPrefMember hide_brave_vpn_button_by_policy_;
#endif

  BooleanPrefMember show_bookmarks_button_;

  raw_ptr<AIChatButton> ai_chat_button_ = nullptr;
  BooleanPrefMember show_ai_chat_button_;
  BooleanPrefMember hide_ai_chat_button_by_policy_;

  BooleanPrefMember show_wallet_button_;
  BooleanPrefMember wallet_private_window_enabled_;

  BooleanPrefMember location_bar_is_wide_;

  BooleanPrefMember show_vertical_tabs_;
  BooleanPrefMember show_title_bar_on_vertical_tabs_;
#if BUILDFLAG(IS_LINUX)
  BooleanPrefMember use_custom_chrome_frame_;
#endif  // BUILDFLAG(IS_LINUX)

  // Whether this toolbar has been initialized.
  bool brave_initialized_ = false;
  // Tracks profile count to determine whether profile switcher should be shown.
  base::ScopedObservation<ProfileAttributesStorage,
                          ProfileAttributesStorage::Observer>
      profile_observer_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_TOOLBAR_VIEW_H_
