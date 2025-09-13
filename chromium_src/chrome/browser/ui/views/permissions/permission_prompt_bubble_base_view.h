// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PERMISSIONS_PERMISSION_PROMPT_BUBBLE_BASE_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PERMISSIONS_PERMISSION_PROMPT_BUBBLE_BASE_VIEW_H_

#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "ui/base/ui_base_types.h"
#include "ui/views/widget/widget_observer.h"

class PermissionPromptBubbleBaseView;

// This class manages z-order of permission prompt bubble and its parent
class PermissionPromptBubbleZOrderManager : public views::WidgetObserver {
 public:
  explicit PermissionPromptBubbleZOrderManager(
      PermissionPromptBubbleBaseView& permission_prompt_bubble);
  PermissionPromptBubbleZOrderManager(
      const PermissionPromptBubbleZOrderManager&) = delete;
  PermissionPromptBubbleZOrderManager& operator=(
      const PermissionPromptBubbleZOrderManager&) = delete;
  ~PermissionPromptBubbleZOrderManager() override;

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;

 private:
  // Sets the zorder for permission prompt bubble to kSecuritySurface, so that
  // it appears above other UI elements even they are floating on top. For
  // example, Picture-in-Picture window is on top of other widgets, but
  // permission prompt bubble should still be on top of it.
  void ElevateZOrder();

  // Restores z-order of widget and its parent widget to the level before
  // elevation.
  void RestoreZOrder();

  raw_ref<PermissionPromptBubbleBaseView> permission_prompt_bubble_;  // owner

  bool z_order_elevated_ = false;

  ui::ZOrderLevel widget_z_order_level_;
  ui::ZOrderLevel parent_widget_z_order_level_;
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      prompt_widget_observation_{this};
};

// Add PermissionPromptBubbleZOrderManager as member variable in order to manage
// widget's z-order level.
#define ShowWidget                                                       \
  ShowWidget_Unused();                                                   \
                                                                         \
 private:                                                                \
  std::unique_ptr<PermissionPromptBubbleZOrderManager> z_order_manager_; \
                                                                         \
 public:                                                                 \
  void ShowWidget

#include <chrome/browser/ui/views/permissions/permission_prompt_bubble_base_view.h>  // IWYU pragma: export

#undef ShowWidget

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PERMISSIONS_PERMISSION_PROMPT_BUBBLE_BASE_VIEW_H_
