// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_REWARDS_ACTION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_REWARDS_ACTION_VIEW_H_

#include <memory>
#include <string>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_rewards/rewards_tab_helper.h"
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "components/prefs/pref_change_registrar.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/views/widget/widget_observer.h"

class Profile;
class TabStripModel;
class WebUIBubbleManager;

// A button that lives in the actions container and opens the Rewards panel. The
// button has an associated context menu and can be hidden by user settings.
class BraveRewardsActionView
    : public ToolbarButton,
      public views::WidgetObserver,
      public TabStripModelObserver,
      public brave_rewards::RewardsTabHelper::Observer,
      public brave_rewards::RewardsPanelCoordinator::Observer,
      public brave_rewards::RewardsServiceObserver,
      public brave_rewards::RewardsNotificationServiceObserver {
  METADATA_HEADER(BraveRewardsActionView, ToolbarButton)
 public:
  explicit BraveRewardsActionView(
      BrowserWindowInterface* browser_window_interface);

  ~BraveRewardsActionView() override;

  BraveRewardsActionView(const BraveRewardsActionView&) = delete;
  BraveRewardsActionView& operator=(const BraveRewardsActionView&) = delete;

  void Update();

  void ClosePanelForTesting();

  // views::View:
  gfx::Rect GetAnchorBoundsInScreen() const override;

  // views::LabelButton:
  std::unique_ptr<views::LabelButtonBorder> CreateDefaultBorder()
      const override;
  void OnThemeChanged() override;

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  // brave_rewards::RewardsTabHelper::Observer:
  void OnPublisherForTabUpdated(const std::string& publisher_id) override;

  // brave_rewards::RewardsPanelCoordinator::Observer:
  void OnRewardsPanelRequested(
      const brave_rewards::mojom::RewardsPanelArgs& args) override;

  // brave_rewards::RewardsServiceObserver:
  void OnPublisherRegistryUpdated() override;

  void OnPublisherUpdated(const std::string& publisher_id) override;

  // brave_rewards::RewardsNotificationServiceObserver:
  void OnNotificationAdded(
      brave_rewards::RewardsNotificationService* service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
          notification) override;

  void OnNotificationDeleted(
      brave_rewards::RewardsNotificationService* service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
          notification) override;

 private:
  void OnButtonPressed();
  void OnPreferencesChanged(const std::string& key);
  content::WebContents* GetActiveWebContents();
  brave_rewards::RewardsService* GetRewardsService();
  brave_rewards::RewardsNotificationService* GetNotificationService();
  bool IsPanelOpen();
  void ToggleRewardsPanel();
  gfx::ImageSkia GetRewardsIcon();
  std::pair<std::string, SkColor> GetBadgeTextAndBackground();
  size_t GetRewardsNotificationCount();
  bool UpdatePublisherStatus();
  void IsPublisherRegisteredCallback(const std::string& publisher_id,
                                     bool is_registered);
  void UpdateTabHelper(content::WebContents* web_contents);

  using WidgetObservation =
      base::ScopedObservation<views::Widget, views::WidgetObserver>;

  using RewardsObservation =
      base::ScopedObservation<brave_rewards::RewardsService,
                              brave_rewards::RewardsServiceObserver>;

  using NotificationServiceObservation = base::ScopedObservation<
      brave_rewards::RewardsNotificationService,
      brave_rewards::RewardsNotificationServiceObserver>;

  raw_ptr<BrowserWindowInterface> browser_window_interface_ = nullptr;
  raw_ptr<brave_rewards::RewardsPanelCoordinator> panel_coordinator_ = nullptr;
  raw_ptr<brave_rewards::RewardsTabHelper> tab_helper_ = nullptr;
  std::unique_ptr<WebUIBubbleManager> bubble_manager_;
  PrefChangeRegistrar pref_change_registrar_;
  std::pair<std::string, bool> publisher_registered_;
  brave_rewards::RewardsTabHelper::Observation tab_helper_observation_{this};
  brave_rewards::RewardsPanelCoordinator::Observation panel_observation_{this};
  WidgetObservation bubble_observation_{this};
  RewardsObservation rewards_service_observation_{this};
  NotificationServiceObservation notification_service_observation_{this};
  base::WeakPtrFactory<BraveRewardsActionView> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_REWARDS_ACTION_VIEW_H_
