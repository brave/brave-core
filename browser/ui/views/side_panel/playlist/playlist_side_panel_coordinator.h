/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_PLAYLIST_PLAYLIST_SIDE_PANEL_COORDINATOR_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_PLAYLIST_PLAYLIST_SIDE_PANEL_COORDINATOR_H_

#include <memory>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/views/side_panel/playlist/playlist_contents_wrapper.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"

namespace playlist {
class PlaylistUI;
FORWARD_DECLARE_TEST(PlaylistBrowserTest, PanelToggleTestWhilePlaying);
}  // namespace playlist

class BrowserWindowInterface;
class SidePanelRegistry;
class SidePanelEntryScope;
class SidePanelWebUIView;

namespace sidebar {
class SidebarController;
}  // namespace sidebar

class PlaylistSidePanelCoordinator : public views::ViewObserver {
 public:
  class Proxy : public content::WebContentsUserData<Proxy> {
   public:
    ~Proxy() override;

    base::WeakPtr<PlaylistSidePanelCoordinator> GetCoordinator();

   private:
    friend WebContentsUserData;

    Proxy(content::WebContents* web_contents,
          base::WeakPtr<PlaylistSidePanelCoordinator> coordinator);

    base::WeakPtr<PlaylistSidePanelCoordinator> coordinator_;

    WEB_CONTENTS_USER_DATA_KEY_DECL();
  };

  PlaylistSidePanelCoordinator(BrowserWindowInterface* browser,
                               sidebar::SidebarController* sidebar_controller,
                               Profile* profile);
  PlaylistSidePanelCoordinator(const PlaylistSidePanelCoordinator&) = delete;
  PlaylistSidePanelCoordinator& operator=(const PlaylistSidePanelCoordinator&) =
      delete;
  ~PlaylistSidePanelCoordinator() override;

  void CreateAndRegisterEntry(SidePanelRegistry* global_registry);

  WebUIContentsWrapperT<playlist::PlaylistUI>* contents_wrapper() {
    return contents_wrapper_.get();
  }

  void ActivatePanel();
  void LoadPlaylist(const std::string& playlist_id, const std::string& item_id);

  SidePanelWebUIView* side_panel_web_view() { return side_panel_web_view_; }

  BrowserView* GetBrowserView();

  // views::ViewObserver:
  void OnViewIsDeleting(views::View* view) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(playlist::PlaylistBrowserTest,
                           PanelToggleTestWhilePlaying);

  std::unique_ptr<views::View> CreateWebView(SidePanelEntryScope& scope);

  const raw_ptr<BrowserWindowInterface> browser_;
  const raw_ptr<sidebar::SidebarController> sidebar_controller_;
  const raw_ptr<Profile> profile_;

  bool is_audible_for_testing_ = false;
  std::unique_ptr<PlaylistContentsWrapper> contents_wrapper_;

  raw_ptr<SidePanelWebUIView> side_panel_web_view_;

  base::ScopedObservation<views::View, views::ViewObserver> view_observation_{
      this};

  base::WeakPtrFactory<PlaylistSidePanelCoordinator> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_PLAYLIST_PLAYLIST_SIDE_PANEL_COORDINATOR_H_
