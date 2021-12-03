/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_ACTIONS_CONTAINER_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_ACTIONS_CONTAINER_H_

#include <map>
#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/extensions/api/brave_action_api.h"
#include "brave/browser/ui/views/brave_actions/brave_rewards_action_stub_view.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/browser/extensions/api/extension_action/extension_action_api.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/extensions/extensions_container.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_controller.h"
#include "chrome/browser/ui/views/toolbar/toolbar_action_view.h"
#include "components/prefs/pref_member.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/common/extension.h"
#include "ui/gfx/skia_util.h"
#include "ui/views/view.h"

class BraveActionViewController;
class BraveActionsContainerTest;
class BraveShieldsActionView;
class RewardsBrowserTest;

namespace extensions {
class ExtensionActionManager;
}

namespace views {
class Button;
}

// This View contains all the built-in BraveActions such as Shields and Payments
// TODO(petemill): consider splitting to separate model, like
// ToolbarActionsModel and ToolbarActionsBar
class BraveActionsContainer : public views::View,
                              public ExtensionsContainer,
                              public extensions::BraveActionAPI::Observer,
                              public extensions::ExtensionActionAPI::Observer,
                              public extensions::ExtensionRegistryObserver,
                              public ToolbarActionView::Delegate,
                              public BraveRewardsActionStubView::Delegate {
 public:
  BraveActionsContainer(Browser* browser, Profile* profile);
  BraveActionsContainer(const BraveActionsContainer&) = delete;
  BraveActionsContainer& operator=(const BraveActionsContainer&) = delete;
  ~BraveActionsContainer() override;
  void Init();
  void Update();
  void SetShouldHide(bool should_hide);
  // ToolbarActionView::Delegate
  content::WebContents* GetCurrentWebContents() override;
  // Returns the view of the toolbar actions overflow menu to use as a
  // reference point for a popup when this view isn't visible.
  views::LabelButton* GetOverflowReferenceView() const override;
  // Returns the preferred size of the ToolbarActionView.
  gfx::Size GetToolbarActionSize() override;
  // Overridden from views::DragController (required by
  // ToolbarActionView::Delegate):
  void WriteDragDataForView(View* sender,
                            const gfx::Point& press_pt,
                            ui::OSExchangeData* data) override;
  int GetDragOperationsForView(View* sender, const gfx::Point& p) override;
  bool CanStartDragForView(View* sender,
                           const gfx::Point& press_pt,
                           const gfx::Point& p) override;

  // BraveRewardsActionStubView::Delegate
  void OnRewardsStubButtonClicked() override;

  // ExtensionRegistryObserver:
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const extensions::Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const extensions::Extension* extension,
                           extensions::UnloadedExtensionReason reason) override;

  // ExtensionActionAPI::Observer
  // Called when there is a change to the given |extension_action|.
  // |web_contents| is the web contents that was affected, and
  // |browser_context| is the associated BrowserContext. (The latter is
  // included because ExtensionActionAPI is shared between normal and
  // incognito contexts, so |browser_context| may not equal
  // |browser_context_|.)
  void OnExtensionActionUpdated(
      extensions::ExtensionAction* extension_action,
      content::WebContents* web_contents,
      content::BrowserContext* browser_context) override;

  // Brave Rewards preferences change observers callback.
  void OnBraveRewardsPreferencesChanged();

  // views::View:
  void ChildPreferredSizeChanged(views::View* child) override;

 private:
  friend class ::BraveActionsContainerTest;
  friend class ::RewardsBrowserTest;

  // Special positions in the container designators
  enum ActionPosition : int {
    ACTION_ANY_POSITION = -1,
  };

  // Action info container
  struct BraveActionInfo {
    BraveActionInfo();
    ~BraveActionInfo();
    // Reset view and view controller
    void Reset();

    int position_;
    std::unique_ptr<views::Button> view_;
    std::unique_ptr<BraveActionViewController> view_controller_;
  };

  // Actions that belong to the container
  std::map<std::string, BraveActionInfo> actions_;

  // ExtensionsContainer:
  ToolbarActionViewController* GetActionForId(
      const std::string& action_id) override;
  ToolbarActionViewController* GetPoppedOutAction() const override;
  void OnContextMenuShown(ToolbarActionViewController* extension) override;
  void OnContextMenuClosed(ToolbarActionViewController* extension) override;
  bool IsActionVisibleOnToolbar(
      const ToolbarActionViewController* action) const override;
  extensions::ExtensionContextMenuModel::ButtonVisibility GetActionVisibility(
      const ToolbarActionViewController* action) const override;
  void UndoPopOut() override;
  void SetPopupOwner(ToolbarActionViewController* popup_owner) override;
  void HideActivePopup() override;
  bool CloseOverflowMenuIfOpen() override;
  void PopOutAction(ToolbarActionViewController* action,
                    bool is_sticky,
                    base::OnceClosure closure) override;
  bool ShowToolbarActionPopupForAPICall(const std::string& action_id) override;
  void ShowToolbarActionBubble(
      std::unique_ptr<ToolbarActionsBarBubbleDelegate> bubble) override;
  void ShowToolbarActionBubbleAsync(
      std::unique_ptr<ToolbarActionsBarBubbleDelegate> bubble) override;
  void ToggleExtensionsMenu() override;
  bool HasAnyExtensions() const override;

  // Actions operations
  bool ShouldShowAction(const std::string& id) const;
  bool IsContainerAction(const std::string& id) const;
  void AddAction(const extensions::Extension* extension);
  void AddAction(const std::string& id);
  bool ShouldShowBraveRewardsAction() const;
  void AddActionStubForRewards();
  void AddActionViewForShields();
  void RemoveAction(const std::string& id);
  void UpdateActionVisibility(const std::string& id);
  views::Button* GetActionButton(const std::string& id) const;
  bool IsActionShown(const std::string& id) const;
  void UpdateActionState(const std::string& id);
  void AttachAction(const std::string& id);

  // BraveActionAPI::Observer
  void OnBraveActionShouldTrigger(const std::string& extension_id,
      std::unique_ptr<std::string> ui_relative_path) override;

  bool should_hide_ = false;

  bool is_rewards_pressed_ = false;

  ToolbarActionViewController* popup_owner_ = nullptr;

  // The Browser this LocationBarView is in.  Note that at least
  // chromeos::SimpleWebViewDialog uses a LocationBarView outside any browser
  // window, so this may be NULL.
  Browser* const browser_;

  void OnExtensionSystemReady();

  raw_ptr<extensions::ExtensionSystem> extension_system_ = nullptr;
  raw_ptr<extensions::ExtensionActionAPI> extension_action_api_ = nullptr;
  raw_ptr<extensions::ExtensionRegistry> extension_registry_ = nullptr;
  raw_ptr<extensions::ExtensionActionManager> extension_action_manager_ =
      nullptr;
  raw_ptr<extensions::BraveActionAPI> brave_action_api_ = nullptr;

  // Listen to extension load, unloaded notifications.
  base::ScopedObservation<extensions::ExtensionRegistry,
                          extensions::ExtensionRegistryObserver>
      extension_registry_observer_{this};

  // Listen to when the action is updated
  base::ScopedObservation<extensions::ExtensionActionAPI,
                          extensions::ExtensionActionAPI::Observer>
      extension_action_observer_{this};

  // Listen to when we need to open a popup
  base::ScopedObservation<extensions::BraveActionAPI,
                          extensions::BraveActionAPI::Observer>
      brave_action_observer_{this};

  BraveShieldsActionView* shields_action_btn_ = nullptr;

  // Listen for Brave Rewards preferences changes.
  BooleanPrefMember brave_rewards_enabled_;
  BooleanPrefMember show_brave_rewards_button_;

  raw_ptr<brave_rewards::RewardsService> rewards_service_ = nullptr;

  base::WeakPtrFactory<BraveActionsContainer> weak_ptr_factory_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_ACTIONS_CONTAINER_H_
