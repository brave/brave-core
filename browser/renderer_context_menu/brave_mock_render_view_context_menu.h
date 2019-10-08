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

#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/renderer_context_menu/render_view_context_menu_proxy.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/gfx/image/image.h"

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
    base::string16 title;
    bool is_submenu;  // This item lives in a submenu.
    bool has_submenu;  // This item is a submenu.
  };

  explicit BraveMockRenderViewContextMenu(Profile* profile);
  ~BraveMockRenderViewContextMenu() override;

  // SimpleMenuModel::Delegate implementation.
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

  // RenderViewContextMenuProxy implementation.
  void AddMenuItem(int command_id, const base::string16& title) override;
  void AddMenuItemWithIcon(int command_id,
                           const base::string16& title,
                           const gfx::ImageSkia& image) override;
  void AddMenuItemWithIcon(int command_id,
                           const base::string16& title,
                           const gfx::VectorIcon& image) override;
  void AddCheckItem(int command_id, const base::string16& title) override;
  void AddSeparator() override;
  void AddSubMenu(int command_id,
                  const base::string16& label,
                  ui::MenuModel* model) override;
  void AddSubMenuWithStringIdAndIcon(int command_id,
                                     int message_id,
                                     ui::MenuModel* model,
                                     const gfx::ImageSkia& image) override;
  void AddSubMenuWithStringIdAndIcon(int command_id,
                                     int message_id,
                                     ui::MenuModel* model,
                                     const gfx::VectorIcon& image) override;
  void UpdateMenuItem(int command_id,
                      bool enabled,
                      bool hidden,
                      const base::string16& title) override;
  void UpdateMenuIcon(int command_id, const gfx::Image& image) override;
  void RemoveMenuItem(int command_id) override;
  void RemoveAdjacentSeparators() override;
  void AddSpellCheckServiceItem(bool is_checked) override;
  void AddAccessibilityLabelsServiceItem(bool is_checked) override;
  content::RenderViewHost* GetRenderViewHost() const override;
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
  RenderViewContextMenuObserver* observer_;

  // Either a regular profile or an incognito profile.
  Profile* profile_;

  // A list of menu items added.
  std::vector<MockMenuItem> items_;

  // Is menu printing enabled.
  bool enable_print_menu_;

  DISALLOW_COPY_AND_ASSIGN(BraveMockRenderViewContextMenu);
};

#endif  // BRAVE_BROWSER_RENDERER_CONTEXT_MENU_BRAVE_MOCK_RENDER_VIEW_CONTEXT_MENU_H_
