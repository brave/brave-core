// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_ACTION_PAGE_ACTION_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_ACTION_PAGE_ACTION_CONTROLLER_H_

namespace gfx {
class Insets;
}  // namespace gfx

namespace page_actions {
class PageActionControllerImpl;
}  // namespace page_actions

#include <chrome/browser/ui/page_action/page_action_controller.h>  // IWYU pragma: export

namespace page_actions {

// Brave override of the upstream concrete page-action controller. Used by
// PartitionedStoragePageActionController to drive Brave-specific page actions
// (chip colors, label, height, triggerable event).
class PageActionControllerImpl
    : public chromium_impl::PageActionControllerImpl {
 public:
  using chromium_impl::PageActionControllerImpl::PageActionControllerImpl;
  ~PageActionControllerImpl() override;

  void SetAlwaysShowLabel(actions::ActionId action_id, bool always_show);
  void OverrideChipColors(actions::ActionId action_id,
                          std::optional<SkColor> override_background_color,
                          std::optional<SkColor> override_foreground_color);
  void ClearOverrideChipColors(actions::ActionId action_id);
  void SetOverrideHeight(actions::ActionId action_id, int height);
  void ClearOverrideHeight(actions::ActionId action_id);
  void SetOverrideTriggerableEvent(actions::ActionId action_id,
                                   std::optional<int> event_flags);
  void SetOverrideBorder(actions::ActionId action_id,
                         const gfx::Insets& border);
  void ClearOverrideBorder(actions::ActionId action_id);

  // chromium_impl::PageActionControllerImpl:
  std::unique_ptr<PageActionModelInterface> CreateModel(
      actions::ActionId action_id,
      bool is_ephemeral) override;
};

}  // namespace page_actions

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_ACTION_PAGE_ACTION_CONTROLLER_H_
