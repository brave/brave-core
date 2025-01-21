/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_SYSTEM_MENU_MODEL_BUILDER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_SYSTEM_MENU_MODEL_BUILDER_H_

#include "chrome/browser/ui/views/frame/system_menu_model_builder.h"

// This class can be used to update toolbar/frame context menus.
class BraveSystemMenuModelBuilder : public SystemMenuModelBuilder {
 public:
  BraveSystemMenuModelBuilder(ui::AcceleratorProvider* provider,
                              Browser* browser);
  virtual ~BraveSystemMenuModelBuilder();

 private:
  // SystemMenuModelBuilder
  void BuildSystemMenuForBrowserWindow(ui::SimpleMenuModel* model) override;

  void InsertBraveSystemMenuForBrowserWindow(ui::SimpleMenuModel* model);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_SYSTEM_MENU_MODEL_BUILDER_H_
