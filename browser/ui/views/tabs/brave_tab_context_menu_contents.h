/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTEXT_MENU_CONTENTS_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTEXT_MENU_CONTENTS_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/base/ui_base_types.h"

class BraveBrowserTabStripController;
class BraveTabMenuModel;
class Browser;
class Tab;

namespace sessions {
class TabRestoreService;
}  // namespace sessions

namespace views {
class MenuRunner;
}  // namespace views

class BraveTabContextMenuContents : public ui::SimpleMenuModel::Delegate {
 public:
  BraveTabContextMenuContents(Tab* tab,
                              BraveBrowserTabStripController* controller,
                              int index);
  BraveTabContextMenuContents(const BraveTabContextMenuContents&) = delete;
  BraveTabContextMenuContents& operator=(const BraveTabContextMenuContents&) =
      delete;
  ~BraveTabContextMenuContents() override;

  void Cancel();

  void RunMenuAt(const gfx::Point& point, ui::MenuSourceType source_type);

  // ui::SimpleMenuModel::Delegate overrides:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  bool GetAcceleratorForCommandId(int command_id,
                                  ui::Accelerator* accelerator) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

 private:
  bool IsBraveCommandIdEnabled(int command_id) const;
  void ExecuteBraveCommand(int command_id);
  bool IsBraveCommandId(int command_id) const;

  std::unique_ptr<BraveTabMenuModel> model_;
  std::unique_ptr<views::MenuRunner> menu_runner_;

  raw_ptr<Tab> tab_ = nullptr;
  raw_ptr<Browser> browser_ = nullptr;
  sessions::TabRestoreService* restore_service_ = nullptr;
  raw_ptr<BraveBrowserTabStripController> controller_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTEXT_MENU_CONTENTS_H_
