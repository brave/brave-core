/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_REWARDS_BUBBLE_MANAGER_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_REWARDS_BUBBLE_MANAGER_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"

#include "ui/gfx/geometry/rect.h"

class Profile;

namespace views {
class View;
}  // namespace views

// Bubble manager for Rewards panel
class BraveRewardsBubbleManager : public WebUIBubbleManager {
 public:
  BraveRewardsBubbleManager(views::View* anchor_view, Profile* profile);

  ~BraveRewardsBubbleManager() override;

  BraveRewardsBubbleManager(const BraveRewardsBubbleManager&) = delete;
  BraveRewardsBubbleManager& operator=(const BraveRewardsBubbleManager&) =
      delete;

  // WebUIBubbleManager:
  void MaybeInitPersistentRenderer() override {}
  base::WeakPtr<WebUIBubbleDialogView> CreateWebUIBubbleDialog(
      const absl::optional<gfx::Rect>& anchor) override;

  base::WeakPtr<WebUIBubbleDialogView> bubble_view() { return bubble_view_; }

 private:
  const raw_ptr<views::View> anchor_view_;
  const raw_ptr<Profile> profile_;
  base::WeakPtr<WebUIBubbleDialogView> bubble_view_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_REWARDS_BUBBLE_MANAGER_H_
