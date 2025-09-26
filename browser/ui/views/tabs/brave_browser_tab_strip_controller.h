/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_BROWSER_TAB_STRIP_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_BROWSER_TAB_STRIP_CONTROLLER_H_

#include <memory>
#include <optional>
#include <vector>

#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"
#include "ui/views/controls/textfield/textfield_controller.h"
namespace ui {
class SimpleMenuModel;
}
namespace views {
class MenuRunner;
class Widget;
}

class BraveTabContextMenuContents;

class BraveBrowserTabStripController : public BrowserTabStripController,
                                       public ui::SimpleMenuModel::Delegate,
                                       public views::TextfieldController {
 public:
  BraveBrowserTabStripController(TabStripModel* model,
                                 BrowserView* browser_view,
                                 std::unique_ptr<TabMenuModelFactory>
                                     menu_model_factory_override = nullptr);
  BraveBrowserTabStripController(const BraveBrowserTabStripController&) =
      delete;
  BraveBrowserTabStripController& operator=(
      const BraveBrowserTabStripController&) = delete;
  ~BraveBrowserTabStripController() override;

  const std::optional<int> GetModelIndexOf(Tab* tab);

  // Enters rename mode for the tab at the given index. This only affects UI
  // side.
  void EnterTabRenameModeAt(int index);

  // Sets the custom title for the tab at the specified index.
  void SetCustomTitleForTab(int index,
                            const std::optional<std::u16string>& title);

  // Opens the emoji picker bubble for the tab at the specified index.
  void OpenEmojiPickerForTab(int index);

  // Sets the custom emoji favicon for the tab at the specified index.
  void SetCustomEmojiFaviconForTab(
      int index, const std::optional<std::u16string>& emoji);

  // BrowserTabStripController overrides:
  void ShowContextMenuForTab(Tab* tab,
                             const gfx::Point& p,
                             ui::mojom::MenuSourceType source_type) override;
  void ExecuteCommandForTab(TabStripModel::ContextMenuCommand command_id,
                            const Tab* tab) override;

  // ui::SimpleMenuModel::Delegate for emoji picker menu
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

  // views::TextfieldController for native emoji capture
  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;

 private:
  void ShowEmojiPickerMenu(int model_index);
  void ShowNativeEmojiPickerWithCapture(int model_index);

  // If non-NULL it means we're showing a menu for the tab.
  std::unique_ptr<BraveTabContextMenuContents> context_menu_contents_;

  // Emoji picker state
  std::unique_ptr<ui::SimpleMenuModel> emoji_menu_model_;
  std::unique_ptr<views::MenuRunner> emoji_menu_runner_;
  std::vector<std::u16string> emoji_items_;
  int emoji_target_model_index_ = -1;

  // Native picker capture field
  raw_ptr<views::Textfield> emoji_capture_field_ = nullptr;
  int emoji_capture_model_index_ = -1;
  raw_ptr<views::Widget> emoji_capture_widget_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_BROWSER_TAB_STRIP_CONTROLLER_H_
