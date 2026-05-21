// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_ACTION_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_ACTION_CONTROLLER_H_

#include <memory>
#include <optional>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/label_button.h"

class IconWithBadgeImageSource;
class TabStripModel;

namespace content {
class WebContents;
}  // namespace content

namespace views {
class View;
class Widget;
}  // namespace views

class BraveShieldsActionController
    : public brave_shields::BraveShieldsTabHelper::Observer,
      public TabStripModelObserver {
 public:
  enum class IconStyle {
    kLocationBar,
    kWebAppTitleBar,
  };

  using CreateWebUIBubbleManagerCallback =
      base::RepeatingCallback<std::unique_ptr<WebUIBubbleManager>(
          views::View* anchor_view,
          BrowserWindowInterface* browser_window_interface,
          const GURL& webui_url,
          int task_manager_string_id,
          bool force_load_on_create)>;

  BraveShieldsActionController(
      BrowserWindowInterface* bwi,
      CreateWebUIBubbleManagerCallback create_bubble_manager_callback);
  BraveShieldsActionController(const BraveShieldsActionController&) = delete;
  BraveShieldsActionController& operator=(const BraveShieldsActionController&) =
      delete;
  ~BraveShieldsActionController() override;

  void SetAnchorView(views::View* anchor);
  // Invoked when icon/badge or tooltip should be refreshed.
  void SetOnStateChanged(base::RepeatingClosure callback);
  void SetIconStyle(IconStyle style) { icon_style_ = style; }

  // Updates |button| image for its current preferred size.
  void RefreshButtonImages(views::LabelButton* button);
  ui::ImageModel GetImageModel(const gfx::Size& preferred_size) const;
  std::u16string GetTooltipText() const;
  void OnButtonPressed();
  views::Widget* GetBubbleWidget();

  // TabStripModelObserver
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  // brave_shields::BraveShieldsTabHelper::Observer
  void OnResourcesChanged() override;
  void OnShieldsEnabledChanged() override;
  void OnRepeatedReloadsDetected() override;

  Profile* profile() { return &profile_.get(); }

 private:
  bool IsPageInReaderMode(content::WebContents* web_contents) const;
  bool ShouldShowBubbleForContents(content::WebContents* web_contents) const;
  void ShowBubble(GURL webui_url);
  int IconDimensionForLayout() const;

  void RemoveObserverFromWebContents(content::WebContents* contents);
  void AddObserverToWebContentsIfPresent(content::WebContents* contents);
  void NotifyStateChanged();

  gfx::ImageSkia GetIconImage(bool is_enabled) const;
  std::unique_ptr<IconWithBadgeImageSource> GetImageSource(
      const gfx::Size& preferred_size) const;

  const raw_ptr<BrowserWindowInterface> browser_window_interface_ = nullptr;
  CreateWebUIBubbleManagerCallback create_bubble_manager_callback_;

  raw_ptr<views::View> anchor_view_ = nullptr;
  raw_ref<Profile> profile_;
  raw_ref<TabStripModel> tab_strip_model_;

  std::unique_ptr<WebUIBubbleManager> webui_bubble_manager_;
  std::optional<GURL> last_webui_url_;

  IconStyle icon_style_ = IconStyle::kLocationBar;
  base::RepeatingClosure on_state_changed_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_ACTION_CONTROLLER_H_
