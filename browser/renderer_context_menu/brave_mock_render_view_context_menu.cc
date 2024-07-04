/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/renderer_context_menu/brave_mock_render_view_context_menu.h"

#include <iomanip>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

BraveMockRenderViewContextMenu::MockMenuItem::MockMenuItem()
    : command_id(0),
      enabled(false),
      checked(false),
      hidden(false),
      is_submenu(false),
      has_submenu(false) {}

BraveMockRenderViewContextMenu::MockMenuItem::MockMenuItem(
    const MockMenuItem& other) = default;

BraveMockRenderViewContextMenu::MockMenuItem::~MockMenuItem() = default;

BraveMockRenderViewContextMenu::MockMenuItem&
BraveMockRenderViewContextMenu::MockMenuItem::operator=(
    const MockMenuItem& other) = default;

void BraveMockRenderViewContextMenu::MockMenuItem::PrintMockMenuItem(
    unsigned int offset) const {
  std::cout << std::setfill(' ');
  if (offset)
    std::cout << std::setfill(' ') << std::setw(offset) << ' ';
  std::cout << (has_submenu ? "> " : "  ");
  if (command_id == -1) {
    std::cout << std::setfill(' ') << std::setw(9) << ' ';
    std::cout << std::setfill('-') << std::setw(15) << '-';
  } else {
    std::cout << std::setw(8) << command_id;
    std::cout << " " << title;
    if (!enabled || checked || hidden) {
      std::cout << " (";
      if (!enabled)
        std::cout << " disabled ";
      if (checked)
        std::cout << " checked ";
      if (hidden)
        std::cout << " hidden ";
      std::cout << ")";
    }
  }
  std::cout << std::endl;
}

BraveMockRenderViewContextMenu::BraveMockRenderViewContextMenu(Profile* profile)
    : observer_(nullptr), profile_(profile), enable_print_menu_(false) {}

BraveMockRenderViewContextMenu::~BraveMockRenderViewContextMenu() = default;

// SimpleMenuModel::Delegate implementation.

bool BraveMockRenderViewContextMenu::IsCommandIdChecked(int command_id) const {
  return observer_->IsCommandIdChecked(command_id);
}

bool BraveMockRenderViewContextMenu::IsCommandIdEnabled(int command_id) const {
  return observer_->IsCommandIdEnabled(command_id);
}

void BraveMockRenderViewContextMenu::ExecuteCommand(int command_id,
                                                    int event_flags) {
  observer_->ExecuteCommand(command_id);
}

// RenderViewContextMenuProxy implementation.

void BraveMockRenderViewContextMenu::AddMenuItem(int command_id,
                                                 const std::u16string& title) {
  MockMenuItem item;
  item.command_id = command_id;
  item.enabled = observer_->IsCommandIdEnabled(command_id);
  item.checked = false;
  item.hidden = false;
  item.title = title;
  items_.push_back(item);
}

void BraveMockRenderViewContextMenu::AddMenuItemWithIcon(
    int command_id,
    const std::u16string& title,
    const ui::ImageModel& icon) {
  AddMenuItem(command_id, title);
}

void BraveMockRenderViewContextMenu::AddCheckItem(int command_id,
                                                  const std::u16string& title) {
  MockMenuItem item;
  item.command_id = command_id;
  item.enabled = observer_->IsCommandIdEnabled(command_id);
  item.checked = observer_->IsCommandIdChecked(command_id);
  item.hidden = false;
  item.title = title;
  items_.push_back(item);
}

void BraveMockRenderViewContextMenu::AddSeparator() {
  MockMenuItem item;
  item.command_id = -1;
  item.enabled = false;
  item.checked = false;
  item.hidden = false;
  items_.push_back(item);
}

void BraveMockRenderViewContextMenu::AddSubMenu(int command_id,
                                                const std::u16string& label,
                                                ui::MenuModel* model) {
  MockMenuItem item;
  item.command_id = command_id;
  item.enabled = observer_->IsCommandIdEnabled(command_id);
  item.checked = observer_->IsCommandIdChecked(command_id);
  item.hidden = false;
  item.title = label;
  item.has_submenu = true;
  items_.push_back(item);

  for (size_t i = 0; i < model->GetItemCount(); ++i) {
    MockMenuItem sub_item;
    sub_item.is_submenu = true;
    if (model->GetTypeAt(i) != ui::MenuModel::TYPE_SEPARATOR) {
      sub_item.command_id = model->GetCommandIdAt(i);
      sub_item.enabled = observer_->IsCommandIdSupported(sub_item.command_id)
                             ? model->IsEnabledAt(i)
                             : false;
      sub_item.checked = observer_->IsCommandIdSupported(sub_item.command_id)
                             ? model->IsItemCheckedAt(i)
                             : false;
      sub_item.hidden = !model->IsVisibleAt(i);
      sub_item.title = model->GetLabelAt(i);
    } else {
      sub_item.command_id = -1;
    }
    items_.push_back(sub_item);
  }
}

void BraveMockRenderViewContextMenu::AddSubMenuWithStringIdAndIcon(
    int command_id,
    int message_id,
    ui::MenuModel* model,
    const ui::ImageModel& icon) {
  AddSubMenu(command_id, base::NumberToString16(message_id), model);
}

void BraveMockRenderViewContextMenu::UpdateMenuItem(
    int command_id,
    bool enabled,
    bool hidden,
    const std::u16string& title) {
  for (auto& item : items_) {
    if (item.command_id == command_id) {
      item.enabled = enabled;
      item.hidden = hidden;
      item.title = title;
      return;
    }
  }

  FAIL() << "Menu observer is trying to change a menu item it doesn't own."
         << " command_id: " << command_id;
}

void BraveMockRenderViewContextMenu::UpdateMenuIcon(
    int command_id,
    const ui::ImageModel& image) {
  for (auto& item : items_) {
    if (item.command_id == command_id) {
      return;
    }
  }

  FAIL() << "Menu observer is trying to change a menu item it doesn't own."
         << " command_id: " << command_id;
}

void BraveMockRenderViewContextMenu::RemoveMenuItem(int command_id) {
  auto it = items_.begin();
  while (it != items_.end()) {
    if (it->command_id == command_id) {
      bool submenu = it->has_submenu;
      it = items_.erase(it);
      if (submenu) {
        while (it != items_.end() && it->is_submenu)
          it = items_.erase(it);
      }
      break;
    } else {
      ++it;
    }
  }
}

void BraveMockRenderViewContextMenu::RemoveAdjacentSeparators() {}

void BraveMockRenderViewContextMenu::RemoveSeparatorBeforeMenuItem(
    int command_id) {}

void BraveMockRenderViewContextMenu::AddSpellCheckServiceItem(bool is_checked) {
  // Call the static method of RenderViewContextMenu which should our override
  // that doesn't add the item.
  RenderViewContextMenu::AddSpellCheckServiceItem(nullptr, is_checked);
}

void BraveMockRenderViewContextMenu::AddAccessibilityLabelsServiceItem(
    bool is_checked) {}

content::RenderFrameHost* BraveMockRenderViewContextMenu::GetRenderFrameHost()
    const {
  return nullptr;
}

content::BrowserContext* BraveMockRenderViewContextMenu::GetBrowserContext()
    const {
  return profile_;
}

content::WebContents* BraveMockRenderViewContextMenu::GetWebContents() const {
  return nullptr;
}

// Methods that don't implement inherited interfaces.

void BraveMockRenderViewContextMenu::SetObserver(
    RenderViewContextMenuObserver* observer) {
  observer_ = observer;
}

size_t BraveMockRenderViewContextMenu::GetMenuSize() const {
  return items_.size();
}

bool BraveMockRenderViewContextMenu::GetMenuItem(size_t index,
                                                 MockMenuItem* item) const {
  if (index >= items_.size())
    return false;
  *item = items_[index];
  return true;
}

PrefService* BraveMockRenderViewContextMenu::GetPrefs() {
  return profile_->GetPrefs();
}

void BraveMockRenderViewContextMenu::PrintMenu(const std::string& title) const {
  if (!enable_print_menu_)
    return;

  std::cout << title << std::endl;
  std::cout << std::setfill('-') << std::setw(40) << '-' << std::endl;
  for (const auto& item : items_)
    item.PrintMockMenuItem(item.is_submenu ? 4 : 0);
  std::cout << std::setfill('-') << std::setw(40) << '-' << std::endl;
}

void BraveMockRenderViewContextMenu::EnablePrintMenu(bool enable) {
  enable_print_menu_ = enable;
}
