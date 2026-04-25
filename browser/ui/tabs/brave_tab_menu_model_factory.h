/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TAB_MENU_MODEL_FACTORY_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TAB_MENU_MODEL_FACTORY_H_

#include <memory>

#include "chrome/browser/ui/tabs/tab_menu_model_factory.h"

class TabStripModel;
class TabMenuModelDelegate;

namespace brave {

// Factory to create BraveTabMenuModel for tab context menus.
class BraveTabMenuModelFactory : public TabMenuModelFactory {
 public:
  BraveTabMenuModelFactory() = default;
  BraveTabMenuModelFactory(const BraveTabMenuModelFactory&) = delete;
  ~BraveTabMenuModelFactory() override = default;
  BraveTabMenuModelFactory& operator=(const BraveTabMenuModelFactory&) = delete;

  // TabMenuModelFactory:
  std::unique_ptr<ui::SimpleMenuModel> Create(
      ui::SimpleMenuModel::Delegate* delegate,
      TabMenuModelDelegate* tab_menu_model_delegate,
      TabStripModel* tab_strip,
      int index) override;
};

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TAB_MENU_MODEL_FACTORY_H_
