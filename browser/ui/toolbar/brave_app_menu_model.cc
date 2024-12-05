/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_app_menu_model.h"

#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/toolbar/app_menu_icons.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/toolbar/app_menu_model.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/grit/branded_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/models/button_menu_item_model.h"
#include "ui/base/models/menu_separator_types.h"
#include "ui/base/ui_base_features.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/toolbar/brave_vpn_menu_model.h"
#endif

#if BUILDFLAG(ENABLE_COMMANDER)
#include "brave/browser/ui/commander/commander_service.h"
#endif

namespace {

class BraveHelpMenuModel : public ui::SimpleMenuModel {
 public:
  explicit BraveHelpMenuModel(ui::SimpleMenuModel::Delegate* delegate)
      : SimpleMenuModel(delegate) {
    Build();
  }
  BraveHelpMenuModel(const BraveHelpMenuModel&) = delete;
  BraveHelpMenuModel& operator=(const BraveHelpMenuModel&) = delete;
  ~BraveHelpMenuModel() override = default;

 private:
  void Build() {
    AddItemWithStringId(IDC_ABOUT, IDS_ABOUT);
    AddItemWithStringId(IDC_HELP_PAGE_VIA_MENU, IDS_HELP_PAGE);
    AddItemWithStringId(IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER,
                        IDS_SHOW_BRAVE_WEBCOMPAT_REPORTER);
  }
};
}  // namespace
BraveAppMenuModel::BraveAppMenuModel(
    ui::AcceleratorProvider* provider,
    Browser* browser,
    AppMenuIconController* app_menu_icon_controller,
    AlertMenuItem alert_item)
    : AppMenuModel(provider, browser, app_menu_icon_controller, alert_item) {}

BraveAppMenuModel::~BraveAppMenuModel() = default;

#if defined(TOOLKIT_VIEWS)
// static
sidebar::SidebarService::ShowSidebarOption
BraveAppMenuModel::ConvertIDCToSidebarShowOptions(int id) {
  switch (id) {
    case IDC_SIDEBAR_SHOW_OPTION_ALWAYS:
      return sidebar::SidebarService::ShowSidebarOption::kShowAlways;
    case IDC_SIDEBAR_SHOW_OPTION_MOUSEOVER:
      return sidebar::SidebarService::ShowSidebarOption::kShowOnMouseOver;
    case IDC_SIDEBAR_SHOW_OPTION_NEVER:
      return sidebar::SidebarService::ShowSidebarOption::kShowNever;
    default:
      break;
  }
  NOTREACHED();
}
#endif  // defined(TOOLKIT_VIEWS)

void BraveAppMenuModel::Build() {
  // Customize items after build chromium items.
  // Insert & reorder brave menus based on corresponding commands enable status.
  // If we you want to add/remove from app menu, adjust commands enable status
  // at BraveBrowserCommandController.
  AppMenuModel::Build();

  RemoveUpstreamMenus();
  BuildTabsAndWindowsSection();
  BuildBraveProductsSection();
  BuildBrowserSection();
  BuildMoreToolsSubMenu();
  BuildHelpSubMenu();

  ApplyLeoIcons(this);
  ApplyLeoIcons(bookmark_sub_menu_model());

  for (const auto& submenu : sub_menus()) {
    ApplyLeoIcons(submenu.get());
  }

  // TODO(simonhong): apply Nala icons recursively.
  // Can be null in tests.
  if (!bookmark_sub_menu_model()) {
    return;
  }

  if (const auto reading_list_submenu_index =
          bookmark_sub_menu_model()->GetIndexOfCommandId(
              IDC_READING_LIST_MENU)) {
    auto* reading_list_submenu = bookmark_sub_menu_model()->GetSubmenuModelAt(
        *reading_list_submenu_index);
    CHECK(reading_list_submenu);
    ApplyLeoIcons(static_cast<ui::SimpleMenuModel*>(reading_list_submenu));
  }
}

void BraveAppMenuModel::BuildTabsAndWindowsSection() {
  if (IsCommandIdEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE)) {
    InsertItemWithStringIdAt(GetIndexOfCommandId(IDC_NEW_WINDOW).value(),
                             IDC_NEW_TOR_CONNECTION_FOR_SITE,
                             IDS_NEW_TOR_CONNECTION_FOR_SITE);
  }

  if (IsCommandIdEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR)) {
    InsertItemWithStringIdAt(
        GetIndexOfCommandId(IDC_NEW_INCOGNITO_WINDOW).value() + 1,
        IDC_NEW_OFFTHERECORD_WINDOW_TOR, IDS_NEW_OFFTHERECORD_WINDOW_TOR);
  }
}

void BraveAppMenuModel::BuildBraveProductsSection() {
  // Needs to add separator as this section is brave specific section.
  bool need_separator = false;

  if (IsCommandIdEnabled(IDC_TOGGLE_AI_CHAT)) {
    InsertItemWithStringIdAt(GetNextIndexOfBraveProductsSection(),
                             IDC_TOGGLE_AI_CHAT, IDS_TOGGLE_AI_CHAT);
    need_separator = true;
  }

  if (IsCommandIdEnabled(IDC_SHOW_BRAVE_WALLET)) {
    InsertItemWithStringIdAt(GetNextIndexOfBraveProductsSection(),
                             IDC_SHOW_BRAVE_WALLET, IDS_SHOW_BRAVE_WALLET);
    need_separator = true;
  }

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (IsCommandIdEnabled(IDC_BRAVE_VPN_MENU)) {
    sub_menus().push_back(std::make_unique<BraveVPNMenuModel>(
        browser(), browser()->profile()->GetPrefs()));
    InsertSubMenuWithStringIdAt(GetNextIndexOfBraveProductsSection(),
                                IDC_BRAVE_VPN_MENU, IDS_BRAVE_VPN_MENU,
                                sub_menus().back().get());
    need_separator = true;
  } else if (IsCommandIdEnabled(IDC_SHOW_BRAVE_VPN_PANEL)) {
    InsertItemWithStringIdAt(GetNextIndexOfBraveProductsSection(),
                             IDC_SHOW_BRAVE_VPN_PANEL, IDS_BRAVE_VPN_MENU);
    need_separator = true;
  }
#endif

#if defined(TOOLKIT_VIEWS)
  if (sidebar::CanUseSidebar(browser())) {
    auto index = GetNextIndexOfBraveProductsSection();
    // Remove above separator as sidebar will add its top & bottom
    // separator.
    // If |need_separator| is false, there is no item after window section
    // entry and separator. In this case, need to remove that separator because
    // we will add different type of separators around sidebar menu entry.
    if (!need_separator) {
      RemoveItemAt(index - 1);
      index--;
    }

    // Don't need finish this section with more separator as
    // we'll add another separators around sidebar menu entry below.
    need_separator = false;

    sidebar_show_option_model_ = std::make_unique<ui::ButtonMenuItemModel>(
        IDS_APP_MENU_SIDEBAR_TITLE, this);

    sidebar_show_option_model_->AddGroupItemWithStringId(
        IDC_SIDEBAR_SHOW_OPTION_ALWAYS, IDS_APP_MENU_SIDEBAR_ON);
    sidebar_show_option_model_->AddGroupItemWithStringId(
        IDC_SIDEBAR_SHOW_OPTION_MOUSEOVER, IDS_APP_MENU_SIDEBAR_HOVER);
    sidebar_show_option_model_->AddGroupItemWithStringId(
        IDC_SIDEBAR_SHOW_OPTION_NEVER, IDS_APP_MENU_SIDEBAR_OFF);

    // Add additional spacing because LOWER|UPPER separator has more narrow
    // spacing then normal separator.
    InsertSeparatorAt(index++, ui::SPACING_SEPARATOR);
    InsertSeparatorAt(index++, ui::LOWER_SEPARATOR);
    AddButtonItemAt(IDC_SIDEBAR_SHOW_OPTION_MENU,
                    sidebar_show_option_model_.get(),
                    static_cast<size_t>(index++));
    InsertSeparatorAt(index++, ui::UPPER_SEPARATOR);
    InsertSeparatorAt(index, ui::SPACING_SEPARATOR);
  }
#endif
  if (need_separator) {
    InsertSeparatorAt(GetNextIndexOfBraveProductsSection(),
                      ui::NORMAL_SEPARATOR);
  }
}

void BraveAppMenuModel::BuildBrowserSection() {
  // The items are currently arranged as
  // History
  // Downloads
  // Bookmarks
  // ...
  // Extensions (from more tools sub menu)
  // and we want to rearrange to
  // History
  // Bookmarks
  // Downloads
  // Extensions
  std::optional<size_t> bookmark_item_index =
      GetIndexOfCommandId(IDC_BOOKMARKS_MENU);

  // If bookmark is not used, we don't need to adjust download item.
  if (bookmark_item_index.has_value()) {
    // Place download menu under bookmark.
    DCHECK(IsCommandIdEnabled(IDC_SHOW_DOWNLOADS));
    RemoveItemAt(GetIndexOfCommandId(IDC_SHOW_DOWNLOADS).value());
    InsertItemWithStringIdAt(bookmark_item_index.value(), IDC_SHOW_DOWNLOADS,
                             IDS_SHOW_DOWNLOADS);
  }

  // Use this command's enabled state to not having it in guest window.
  // It's disabled in guest window. Upstream's guest window has extensions
  // menu in app menu, but we hide it.
  if (IsCommandIdEnabled(IDC_MANAGE_EXTENSIONS)) {
    // Upstream enabled extensions submenu by default.
    CHECK(features::IsExtensionMenuInRootAppMenu());

    // Use IDC_EXTENSIONS_SUBMENU_MANAGE_EXTENSIONS instead of
    // IDC_MANAGE_EXTENSIONS because executing it from private(tor) window
    // causes crash as LogSafetyHubInteractionMetrics() tries to refer
    // SafetyHubMenuNotificationService. But it's not instantiated in private
    // window. Upstream also has this crash if ExtensionsMenuInAppMenu feature
    // is disabled.
    InsertItemWithStringIdAt(
        GetIndexOfCommandId(IDC_SHOW_DOWNLOADS).value() + 1,
        IDC_EXTENSIONS_SUBMENU_MANAGE_EXTENSIONS, IDS_SHOW_EXTENSIONS);
  }
}

void BraveAppMenuModel::BuildMoreToolsSubMenu() {
  ui::SimpleMenuModel* more_tools_menu_model =
      static_cast<ui::SimpleMenuModel*>(
          GetSubmenuModelAt(GetIndexOfCommandId(IDC_MORE_TOOLS_MENU).value()));
  DCHECK(more_tools_menu_model);

  size_t next_target_index = 0;
  bool need_separator = false;

  // Create New Profile
  if (IsCommandIdEnabled(IDC_ADD_NEW_PROFILE)) {
    more_tools_menu_model->InsertItemWithStringIdAt(
        next_target_index++, IDC_ADD_NEW_PROFILE, IDS_ADD_NEW_PROFILE);
    need_separator = true;
  }

  // Open Guest Window
  if (IsCommandIdEnabled(IDC_OPEN_GUEST_PROFILE)) {
    more_tools_menu_model->InsertItemWithStringIdAt(
        next_target_index++, IDC_OPEN_GUEST_PROFILE, IDS_OPEN_GUEST_PROFILE);
    need_separator = true;
  }

  if (need_separator) {
    more_tools_menu_model->InsertSeparatorAt(next_target_index++,
                                             ui::NORMAL_SEPARATOR);
    need_separator = false;
  }

  // Insert sync menu
  if (IsCommandIdEnabled(IDC_SHOW_BRAVE_SYNC)) {
    more_tools_menu_model->InsertItemWithStringIdAt(
        next_target_index++, IDC_SHOW_BRAVE_SYNC, IDS_SHOW_BRAVE_SYNC);
    need_separator = true;
  }

  if (need_separator) {
    more_tools_menu_model->InsertSeparatorAt(next_target_index++,
                                             ui::NORMAL_SEPARATOR);
    need_separator = false;
  }

  if (!browser()->profile()->IsOffTheRecord()) {
    if (auto index =
            more_tools_menu_model->GetIndexOfCommandId(IDC_NAME_WINDOW)) {
      more_tools_menu_model->InsertItemWithStringIdAt(
          *index + 1, IDC_SHOW_APPS_PAGE, IDS_IDC_SHOW_APPS_PAGE);
    }
  }

#if BUILDFLAG(ENABLE_COMMANDER)
  if (auto index =
          more_tools_menu_model->GetIndexOfCommandId(IDC_NAME_WINDOW)) {
    if (commander::IsEnabled()) {
      more_tools_menu_model->InsertItemWithStringIdAt(*index + 1, IDC_COMMANDER,
                                                      IDS_IDC_COMMANDER);
    }
  }
#endif

  if (auto index =
          more_tools_menu_model->GetIndexOfCommandId(IDC_TASK_MANAGER)) {
    more_tools_menu_model->InsertItemWithStringIdAt(*index, IDC_DEV_TOOLS,
                                                    IDS_DEV_TOOLS);
  }
}

void BraveAppMenuModel::BuildHelpSubMenu() {
  // Put help sub menu above the settings menu.
  if (const auto index = GetIndexOfCommandId(IDC_OPTIONS)) {
    sub_menus().push_back(std::make_unique<BraveHelpMenuModel>(this));
    InsertSubMenuWithStringIdAt(*index, IDC_HELP_MENU, IDS_HELP_MENU,
                                sub_menus().back().get());
  }
}

void BraveAppMenuModel::RemoveUpstreamMenus() {
  ui::SimpleMenuModel* more_tools_model = static_cast<ui::SimpleMenuModel*>(
      GetSubmenuModelAt(GetIndexOfCommandId(IDC_MORE_TOOLS_MENU).value()));
  DCHECK(more_tools_model);

  // Remove upstream's extensions item. It'll be added into top level third
  // section. Upstream enabled extensions submenu by default.
  CHECK(features::IsExtensionMenuInRootAppMenu());
  // Hide extensions sub menu.
  DCHECK(GetIndexOfCommandId(IDC_EXTENSIONS_SUBMENU).has_value());
  RemoveItemAt(GetIndexOfCommandId(IDC_EXTENSIONS_SUBMENU).value());

  {
    // Remove upstream's profile menu. "Add new profile" will be added into more
    // tools sub menu.
    auto index = GetIndexOfCommandId(IDC_PROFILE_MENU_IN_APP_MENU);
    CHECK(index);
    RemoveItemAt(*index);

    // Upstream uses spacing separator after profile menu but we don't use
    // profile menu here. Spacing separator just adds unnecessary space.
    if (GetSeparatorTypeAt(*index) == ui::SPACING_SEPARATOR) {
      RemoveItemAt(*index);
    }
  }

  {
    // Remove upstream's translate item, as it's not hooked up to the
    // translation UI in the omnibox
    auto index = GetIndexOfCommandId(IDC_SHOW_TRANSLATE);
    CHECK(index);
    RemoveItemAt(*index);
  }

  // Remove upstream's dev tools menu and associated separator.
  // It'll be changed its position in more tools.
  if (const auto index = more_tools_model->GetIndexOfCommandId(IDC_DEV_TOOLS)) {
    // If task manager is not existed, just remove separator above the dev tools
    // item. Otherwise, remove separator and item both.
    DCHECK_EQ(ui::MenuModel::TYPE_SEPARATOR,
              more_tools_model->GetTypeAt((*index) - 1));
    if (!more_tools_model->GetIndexOfCommandId(IDC_TASK_MANAGER).has_value()) {
      more_tools_model->RemoveItemAt((*index) - 1);
    } else {
      more_tools_model->RemoveItemAt(*index);
      more_tools_model->RemoveItemAt((*index) - 1);
    }
  }

  // Remove upstream's `Reading Mode` item as we have our own `Speed reader`.
  if (const auto index = more_tools_model->GetIndexOfCommandId(
          IDC_SHOW_READING_MODE_SIDE_PANEL)) {
    more_tools_model->RemoveItemAt(*index);
  }

  // Remove upstream's about menu. It's moved into help sub menu.
  if (const auto index = GetIndexOfCommandId(IDC_ABOUT)) {
    RemoveItemAt(*index);
  }
}

void BraveAppMenuModel::ExecuteCommand(int id, int event_flags) {
#if defined(TOOLKIT_VIEWS)
  if (id == IDC_SIDEBAR_SHOW_OPTION_ALWAYS ||
      id == IDC_SIDEBAR_SHOW_OPTION_MOUSEOVER ||
      id == IDC_SIDEBAR_SHOW_OPTION_NEVER) {
    auto* service =
        sidebar::SidebarServiceFactory::GetForProfile(browser()->profile());
    service->SetSidebarShowOption(ConvertIDCToSidebarShowOptions(id));
    return;
  }
#endif  // defined(TOOLKIT_VIEWS)

  return AppMenuModel::ExecuteCommand(id, event_flags);
}

bool BraveAppMenuModel::IsCommandIdEnabled(int id) const {
  if (id == IDC_EXTENSIONS_SUBMENU_MANAGE_EXTENSIONS) {
    // Always returns true as this command id is only added when it could be
    // used.
    return true;
  }

#if defined(TOOLKIT_VIEWS)
  if (id == IDC_SIDEBAR_SHOW_OPTION_ALWAYS ||
      id == IDC_SIDEBAR_SHOW_OPTION_MOUSEOVER ||
      id == IDC_SIDEBAR_SHOW_OPTION_NEVER) {
    return sidebar::CanUseSidebar(browser());
  }
#endif  // defined(TOOLKIT_VIEWS)

  return AppMenuModel::IsCommandIdEnabled(id);
}

size_t BraveAppMenuModel::GetNextIndexOfBraveProductsSection() const {
  std::vector<int> commands_to_check = {IDC_SHOW_BRAVE_VPN_PANEL,
                                        IDC_BRAVE_VPN_MENU,
                                        IDC_SHOW_BRAVE_WALLET,
                                        IDC_TOGGLE_AI_CHAT,
                                        IDC_NEW_OFFTHERECORD_WINDOW_TOR,
                                        IDC_NEW_INCOGNITO_WINDOW,
                                        IDC_NEW_WINDOW};
  const auto last_index_of_second_section =
      GetProperItemIndex(commands_to_check, false).value();
  const auto last_cmd_id_of_second_section =
      GetCommandIdAt(last_index_of_second_section);

  // If |last_cmd_id_of_second_section| is from new tab & windows section,
  // no item is added to second section yet.
  if (last_cmd_id_of_second_section == IDC_NEW_OFFTHERECORD_WINDOW_TOR ||
      last_cmd_id_of_second_section == IDC_NEW_INCOGNITO_WINDOW ||
      last_cmd_id_of_second_section == IDC_NEW_WINDOW) {
    // Used additional "+1" to skip separator.
    DCHECK_EQ(ui::MenuModel::TYPE_SEPARATOR,
              GetTypeAt(last_index_of_second_section + 1));
    return last_index_of_second_section + 2;
  }

  return last_index_of_second_section + 1;
}

std::optional<size_t> BraveAppMenuModel::GetProperItemIndex(
    std::vector<int> commands_to_check,
    bool insert_next) const {
  const size_t commands_size = commands_to_check.size();
  for (size_t i = 0; i < commands_size; i++) {
    std::optional<size_t> item_index =
        GetIndexOfCommandId(commands_to_check[i]);
    if (item_index.has_value()) {
      return insert_next ? item_index.value() + 1 : item_index;
    }
  }

  NOTREACHED() << "At least, a menu item for this command should exist: "
               << commands_to_check[commands_size - 1];
}
