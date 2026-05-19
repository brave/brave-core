// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_ACTION_PAGE_ACTION_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_ACTION_PAGE_ACTION_MODEL_H_

#include "ui/gfx/geometry/insets.h"

namespace page_actions {
class PageActionModel;
}  // namespace page_actions

#include <chrome/browser/ui/page_action/page_action_model.h>  // IWYU pragma: export

namespace page_actions {

// Brave override of the upstream concrete page-action model. Used by
// PartitionedStoragePageActionController via PageActionControllerImpl to add
// chip-color, label, height and triggerable-event overrides on top of the
// upstream model.
class PageActionModel : public chromium_impl::PageActionModel {
 public:
  using chromium_impl::PageActionModel::PageActionModel;
  ~PageActionModel() override;

  void SetOverrideChipColors(
      PageActionPassKey,
      std::optional<SkColor> override_background_color,
      std::optional<SkColor> override_foreground_color) override;
  void SetAlwaysShowLabel(PageActionPassKey, bool always_show) override;
  void SetOverrideHeight(PageActionPassKey,
                         std::optional<int> height_px) override;
  void SetOverrideTriggerableEvent(PageActionPassKey,
                                   std::optional<int> event_flags) override;
  void SetOverrideBorder(PageActionPassKey,
                         std::optional<gfx::Insets> border) override;
  std::optional<SkColor> GetOverrideBackgroundColor() const override;
  std::optional<SkColor> GetOverrideForegroundColor() const override;
  bool GetAlwaysShowLabel() const override;
  std::optional<int> GetOverrideHeight() const override;
  std::optional<int> GetOverrideTriggerableEvent() const override;
  std::optional<gfx::Insets> GetOverrideBorder() const override;

 private:
  std::optional<SkColor> override_background_color_;
  std::optional<SkColor> override_foreground_color_;
  bool always_show_label_ = false;
  std::optional<int> override_height_;
  std::optional<int> override_triggerable_event_flags_;
  std::optional<gfx::Insets> override_border_;
};

}  // namespace page_actions

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_ACTION_PAGE_ACTION_MODEL_H_
