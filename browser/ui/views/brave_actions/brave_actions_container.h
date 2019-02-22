/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_ACTIONS_CONTAINER_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_ACTIONS_CONTAINER_H_

#include <map>
#include <memory>
#include <string>

#include "chrome/browser/ui/views/toolbar/toolbar_action_view.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_controller.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/extensions/api/extension_action/extension_action_api.h"
#include "components/prefs/pref_member.h"
#include "extensions/common/extension.h"
#include "ui/views/view.h"

class BraveActionsContainerTest;
class BraveRewardsBrowserTest;

namespace extensions {
class ExtensionActionManager;
class ExtensionRegistry;
class ExtensionRegistryObserver;
}

// This View contains all the built-in BraveActions such as Shields and Payments
// TODO(petemill): consider splitting to separate model, like
// ToolbarActionsModel and ToolbarActionsBar
class BraveActionsContainer : public views::View,
                             public extensions::ExtensionActionAPI::Observer,
                             public extensions::ExtensionRegistryObserver,
                             public ToolbarActionView::Delegate {
 public:
  BraveActionsContainer(Browser* browser, Profile* profile);
  ~BraveActionsContainer() override;
  void Init();
  void Update();
  void SetShouldHide(bool should_hide);

  // ToolbarActionView::Delegate
  content::WebContents* GetCurrentWebContents() override;
  bool ShownInsideMenu() const override;
  // Notifies that a drag completed.
  void OnToolbarActionViewDragDone() override;
  // Returns the view of the toolbar actions overflow menu to use as a
  // reference point for a popup when this view isn't visible.
  views::MenuButton* GetOverflowReferenceView() override;
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
      ExtensionAction* extension_action,
      content::WebContents* web_contents,
      content::BrowserContext* browser_context) override;

  // Brave Rewards preferences change observers callback.
  void OnBraveRewardsPreferencesChanged();

  // views::View:
  void ChildPreferredSizeChanged(views::View* child) override;

 private:
  friend class ::BraveActionsContainerTest;
  friend class ::BraveRewardsBrowserTest;
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
    std::unique_ptr<ToolbarActionView> view_;
    std::unique_ptr<ToolbarActionViewController> view_controller_;
  };

  // Actions that belong to the container
  std::map<std::string, BraveActionInfo> actions_;

  // Actions operations
  bool ShouldAddAction(const std::string& id) const;
  bool ShouldAddBraveRewardsAction() const;
  bool IsContainerAction(const std::string& id) const;
  void AddAction(const extensions::Extension* extension,
                 int pos = ACTION_ANY_POSITION);
  void AddAction(const std::string& id, int pos = ACTION_ANY_POSITION);
  void RemoveAction(const std::string& id);
  void ShowAction(const std::string& id, bool show);
  bool IsActionShown(const std::string& id) const;
  void UpdateActionState(const std::string& id);

  bool should_hide_ = false;

  // The Browser this LocationBarView is in.  Note that at least
  // chromeos::SimpleWebViewDialog uses a LocationBarView outside any browser
  // window, so this may be NULL.
  Browser* const browser_;

  void OnExtensionSystemReady();

  extensions::ExtensionActionAPI* extension_action_api_;
  extensions::ExtensionRegistry* extension_registry_;
  extensions::ExtensionActionManager* extension_action_manager_;

  // Listen to extension load, unloaded notifications.
  ScopedObserver<extensions::ExtensionRegistry, ExtensionRegistryObserver>
      extension_registry_observer_;

  // Listen to when the action is updated
  ScopedObserver<extensions::ExtensionActionAPI,
                 extensions::ExtensionActionAPI::Observer>
      extension_action_observer_;

  // Listen for Brave Rewards preferences changes.
  BooleanPrefMember brave_rewards_enabled_;
  BooleanPrefMember hide_brave_rewards_button_;

  base::WeakPtrFactory<BraveActionsContainer> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BraveActionsContainer);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_ACTIONS_CONTAINER_H_
