// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_TEST_SUPPORT_MOCK_PAGE_ACTION_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_TEST_SUPPORT_MOCK_PAGE_ACTION_MODEL_H_

#define MockPageActionModel MockPageActionModel_Chromium
#include <chrome/browser/ui/views/page_action/test_support/mock_page_action_model.h>  // IWYU pragma: export
#undef MockPageActionModel

namespace page_actions {

class MockPageActionModel : public MockPageActionModel_Chromium {
 public:
  MockPageActionModel();
  ~MockPageActionModel() override;

  MOCK_METHOD(void,
              SetAlwaysShowLabel,
              (base::PassKey<PageActionController>, bool),
              (override));
  MOCK_METHOD(void,
              SetOverrideChipColors,
              (base::PassKey<PageActionController>,
               const std::optional<SkColor>&,
               const std::optional<SkColor>&),
              (override));
  MOCK_METHOD(std::optional<SkColor>,
              GetOverrideBackgroundColor,
              (),
              (const, override));
  MOCK_METHOD(std::optional<SkColor>,
              GetOverrideForegroundColor,
              (),
              (const, override));
  MOCK_METHOD(bool, GetAlwaysShowLabel, (), (const, override));
};

}  // namespace page_actions

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_TEST_SUPPORT_MOCK_PAGE_ACTION_MODEL_H_
