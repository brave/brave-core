// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_TEST_SUPPORT_MOCK_PAGE_ACTION_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_TEST_SUPPORT_MOCK_PAGE_ACTION_CONTROLLER_H_

#define MockPageActionController MockPageActionController_Chromium
#include <chrome/browser/ui/views/page_action/test_support/mock_page_action_controller.h>  // IWYU pragma: export
#undef MockPageActionController

namespace page_actions {

class MockPageActionController : public MockPageActionController_Chromium {
 public:
  MockPageActionController();
  ~MockPageActionController() override;

  MOCK_METHOD(void,
              SetAlwaysShowLabel,
              (actions::ActionId action_id, bool always_show),
              (override));
  MOCK_METHOD(void,
              OverrideChipColors,
              (actions::ActionId action_id,
               std::optional<SkColor> override_background_color,
               std::optional<SkColor> override_foreground_color),
              (override));
  MOCK_METHOD(void,
              ClearOverrideChipColors,
              (actions::ActionId action_id),
              (override));
  MOCK_METHOD(void,
              SetOverrideHeight,
              (actions::ActionId action_id, int height_px),
              (override));
  MOCK_METHOD(void,
              ClearOverrideHeight,
              (actions::ActionId action_id),
              (override));
};

}  // namespace page_actions

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_TEST_SUPPORT_MOCK_PAGE_ACTION_CONTROLLER_H_
