/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_SPLIT_TAB_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_SPLIT_TAB_MENU_MODEL_H_

#include <string>

#include "chrome/browser/ui/tabs/split_tab_menu_model.h"

class BraveSplitTabMenuModel : public SplitTabMenuModel {
 public:
  using SplitTabMenuModel::SplitTabMenuModel;
  ~BraveSplitTabMenuModel() override;

  // SplitTabMenuModel:
  std::u16string GetLabelForCommandId(int command_id) const override;
  ui::ImageModel GetIconForCommandId(int command_id) const override;
  bool IsItemForCommandIdDynamic(int command_id) const override;
};

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_SPLIT_TAB_MENU_MODEL_H_
