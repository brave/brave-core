/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_RENDERER_CONTEXT_MENU_BRAVE_MOCK_RENDER_VIEW_CONTEXT_MENU_H_
#define BRAVE_BROWSER_RENDERER_CONTEXT_MENU_BRAVE_MOCK_RENDER_VIEW_CONTEXT_MENU_H_

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "components/renderer_context_menu/render_view_context_menu_proxy.h"
#include "ui/base/models/image_model.h"
#include "ui/base/models/simple_menu_model.h"

class BraveMockRenderViewContextMenu;
class PrefService;
class Profile;
class RenderViewContextMenuObserver;

// A mock context menu proxy used in tests. This class overrides virtual methods
// derived from the RenderViewContextMenuProxy class to monitor calls from a
// MenuObserver class.
class BraveMockRenderViewContextMenu : public ui::SimpleMenuModel::Delegate,
                                       public RenderViewContextMenuProxy {
 public:
  // A menu item used in this test.
  struct MockMenuItem {
    MockMenuItem();
    MockMenuItem(const MockMenuItem& other);
    ~MockMenuItem();

    MockMenuItem& operator=(const MockMenuItem& other);

    void PrintMockMenuItem(unsigned int offset = 0) const;

    int command_id;
    bool enabled;
    bool checked;
    bool hidden;
    std::u16string title;
    bool is_submenu;  // This item lives in a submenu.
    bool has_submenu;  // This item is a submenu.
  };

  explicit BraveMockRenderViewContextMenu(Profile* profile);
  BraveMockRenderViewContextMenu(const BraveMockRenderViewContextMenu&) =
      delete;
  BraveMockRenderViewContextMenu& operator=(
      const BraveMockRenderViewContextMenu&) = delete;
  ~BraveMockRenderViewContextMenu() override;

  // SimpleMenuModel::Delegate implementation.
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

  // RenderViewContextMenuProxy implementation.
  void AddMenuItem(int command_id, const std::u16string& title) override;
  void AddMenuItemWithIcon(int command_id,
                           const std::u16string& title,
                           const ui::ImageModel& icon) override;
  void AddCheckItem(int command_id, const std::u16string& title) override;
  void AddSeparator() override;
  void AddSubMenu(int command_id,
                  const std::u16string& label,
                  ui::MenuModel* model) override;
  void AddSubMenuWithStringIdAndIcon(int command_id,
                                     int message_id,
                                     ui::MenuModel* model,
                                     const ui::ImageModel& icon) override;
  void UpdateMenuItem(int command_id,
                      bool enabled,
                      bool hidden,
                      const std::u16string& title) override;
  void UpdateMenuIcon(int command_id, const ui::ImageModel& image) override;
  void RemoveMenuItem(int command_id) override;
  void RemoveAdjacentSeparators() override;
  void RemoveSeparatorBeforeMenuItem(int command_id) override;
  void AddSpellCheckServiceItem(bool is_checked) override;
  void AddAccessibilityLabelsServiceItem(bool is_checked) override;
  content::RenderFrameHost* GetRenderFrameHost() const override;
  content::WebContents* GetWebContents() const override;
  content::BrowserContext* GetBrowserContext() const override;

  // Attaches a RenderViewContextMenuObserver to be tested.
  void SetObserver(RenderViewContextMenuObserver* observer);

  // Returns the number of items added by the test.
  size_t GetMenuSize() const;

  // Returns the item at |index|.
  bool GetMenuItem(size_t index, MockMenuItem* item) const;

  // Returns the writable profile used in this test.
  PrefService* GetPrefs();

  // Prints the menu to the standard output.
  void PrintMenu(const std::string& title) const;
  void EnablePrintMenu(bool enable = true);

 private:
  // An observer used for initializing the status of menu items added in this
  // test. This is owned by our owner and the owner is responsible for its
  // lifetime.
  raw_ptr<RenderViewContextMenuObserver, DanglingUntriaged> observer_ = nullptr;

  // Either a regular profile or an incognito profile.
  raw_ptr<Profile> profile_ = nullptr;

  // A list of menu items added.
  std::vector<MockMenuItem> items_;

  // Is menu printing enabled.
  bool enable_print_menu_;
};

#endif  // BRAVE_BROWSER_RENDERER_CONTEXT_MENU_BRAVE_MOCK_RENDER_VIEW_CONTEXT_MENU_H_
