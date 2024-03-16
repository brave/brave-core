/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_app_menu_model.h"

#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/toolbar/app_menu_icons.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/grit/branded_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/ui_base_features.h"

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
#include "brave/browser/ipfs/import/ipfs_import_controller.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/browser/ipfs/ipfs_tab_helper.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/keys/ipns_keys_manager.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
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

#if defined(TOOLKIT_VIEWS)
using ShowSidebarOption = sidebar::SidebarService::ShowSidebarOption;

class SidebarMenuModel : public ui::SimpleMenuModel,
                         public ui::SimpleMenuModel::Delegate {
 public:
  explicit SidebarMenuModel(Browser* browser)
      : SimpleMenuModel(this), browser_(browser) {
    Build(browser_);
  }

  ~SidebarMenuModel() override = default;
  SidebarMenuModel(const SidebarMenuModel&) = delete;
  SidebarMenuModel& operator=(const SidebarMenuModel&) = delete;

  // ui::SimpleMenuModel::Delegate overrides:
  void ExecuteCommand(int command_id, int event_flags) override {
    auto* service =
        sidebar::SidebarServiceFactory::GetForProfile(browser_->profile());
    service->SetSidebarShowOption(ConvertIDCToSidebarShowOptions(command_id));
  }

  bool IsCommandIdChecked(int command_id) const override {
    const auto* service =
        sidebar::SidebarServiceFactory::GetForProfile(browser_->profile());
    return ConvertIDCToSidebarShowOptions(command_id) ==
           service->GetSidebarShowOption();
  }

 private:
  void Build(Browser* browser) {
    // IDC_XXX is used instead of direct kShowXXX and it's translated by
    // ConvertIDCToSidebarShowOptions() to avoid any issue with app menu.
    // Ex, id with 0 is always disabled state in the app menu.
    AddCheckItem(IDC_SIDEBAR_SHOW_OPTION_ALWAYS,
                 brave_l10n::GetLocalizedResourceUTF16String(
                     IDS_SIDEBAR_SHOW_OPTION_ALWAYS));
    AddCheckItem(IDC_SIDEBAR_SHOW_OPTION_MOUSEOVER,
                 brave_l10n::GetLocalizedResourceUTF16String(
                     IDS_SIDEBAR_SHOW_OPTION_MOUSEOVER));
    AddCheckItem(IDC_SIDEBAR_SHOW_OPTION_NEVER,
                 brave_l10n::GetLocalizedResourceUTF16String(
                     IDS_SIDEBAR_SHOW_OPTION_NEVER));
  }

  ShowSidebarOption ConvertIDCToSidebarShowOptions(int id) const {
    switch (id) {
      case IDC_SIDEBAR_SHOW_OPTION_ALWAYS:
        return ShowSidebarOption::kShowAlways;
      case IDC_SIDEBAR_SHOW_OPTION_MOUSEOVER:
        return ShowSidebarOption::kShowOnMouseOver;
      case IDC_SIDEBAR_SHOW_OPTION_NEVER:
        return ShowSidebarOption::kShowNever;
      default:
        break;
    }
    NOTREACHED();
    return ShowSidebarOption::kShowAlways;
  }

  raw_ptr<Browser> browser_ = nullptr;
};
#endif

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
// For convenience, we show the last part of the key in the context menu item.
// The length of the key is divided to this constant and the last part is taken.
int kKeyTrimRate = 5;

bool IsIpfsServiceLaunched(content::BrowserContext* browser_context) {
  auto* service = ipfs::IpfsServiceFactory::GetForContext(browser_context);
  return service && service->IsDaemonLaunched();
}

ipfs::IpnsKeysManager* GetIpnsKeysManager(
    content::BrowserContext* browser_context) {
  DCHECK(browser_context);
  auto* service = ipfs::IpfsServiceFactory::GetForContext(browser_context);
  if (!service) {
    return nullptr;
  }
  return service->GetIpnsKeysManager();
}

bool IpnsKeysAvailable(content::BrowserContext* browser_context) {
  auto* keys_manager = GetIpnsKeysManager(browser_context);
  return keys_manager && keys_manager->GetKeys().size();
}

#endif

}  // namespace
BraveAppMenuModel::BraveAppMenuModel(
    ui::AcceleratorProvider* provider,
    Browser* browser,
    AppMenuIconController* app_menu_icon_controller,
    AlertMenuItem alert_item)
    : AppMenuModel(provider, browser, app_menu_icon_controller, alert_item)
#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
      ,
      ipfs_submenu_model_(this),
      ipns_submenu_model_(this)
#endif
{
}

BraveAppMenuModel::~BraveAppMenuModel() = default;

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

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
  if (IsCommandIdEnabled(IDC_APP_MENU_IPFS)) {
    ipfs_submenu_model_.AddItemWithStringId(IDC_APP_MENU_IPFS_SHARE_LOCAL_FILE,
                                            IDS_APP_MENU_IPFS_SHARE_LOCAL_FILE);
    ipfs_submenu_model_.AddItemWithStringId(
        IDC_APP_MENU_IPFS_SHARE_LOCAL_FOLDER,
        IDS_APP_MENU_IPFS_SHARE_LOCAL_FOLDER);
    ipfs_submenu_model_.AddItemWithStringId(IDC_APP_MENU_IPFS_OPEN_FILES,
                                            IDS_APP_MENU_IPFS_OPEN_FILES);
    if (IpnsKeysAvailable(browser()->profile())) {
      ipfs_submenu_model_.InsertSubMenuWithStringIdAt(
          ipfs_submenu_model_.GetItemCount(), IDC_APP_MENU_IPFS_UPDATE_IPNS,
          IDS_APP_MENU_IPFS_UPDATE_IPNS, &ipns_submenu_model_);

      int keys_command_index = IDC_CONTENT_CONTEXT_IMPORT_IPNS_KEYS_START;
      keys_command_index += AddIpfsImportMenuItem(
          IDC_APP_MENU_IPFS_PUBLISH_LOCAL_FILE,
          IDS_APP_MENU_IPFS_PUBLISH_LOCAL_FILE, keys_command_index);
      AddIpfsImportMenuItem(IDC_APP_MENU_IPFS_PUBLISH_LOCAL_FOLDER,
                            IDS_APP_MENU_IPFS_PUBLISH_LOCAL_FOLDER,
                            keys_command_index);
    }
    const int index = GetNextIndexOfBraveProductsSection();
    InsertSubMenuWithStringIdAt(index, IDC_APP_MENU_IPFS, IDS_APP_MENU_IPFS,
                                &ipfs_submenu_model_);

    auto& bundle = ui::ResourceBundle::GetSharedInstance();
    const auto& ipfs_logo = *bundle.GetImageSkiaNamed(IDR_BRAVE_IPFS_LOGO);
    ui::ImageModel image_model = ui::ImageModel::FromImageSkia(ipfs_logo);
    SetIcon(index, image_model);
    need_separator = true;
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

#if defined(TOOLKIT_VIEWS)
  if (sidebar::CanUseSidebar(browser())) {
    sub_menus().push_back(std::make_unique<SidebarMenuModel>(browser()));
    more_tools_menu_model->InsertSubMenuWithStringIdAt(
        next_target_index++, IDC_SIDEBAR_SHOW_OPTION_MENU,
        IDS_SIDEBAR_SHOW_OPTION_TITLE, sub_menus().back().get());
    more_tools_menu_model->InsertSeparatorAt(next_target_index++,
                                             ui::NORMAL_SEPARATOR);
  }
#endif

  if (media_router::MediaRouterEnabled(browser()->profile())) {
    more_tools_menu_model->InsertItemWithStringIdAt(
        next_target_index++, IDC_ROUTE_MEDIA, IDS_MEDIA_ROUTER_MENU_ITEM_TITLE);
    need_separator = true;
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

  // Remove upstream's cast item. It'll be added into more tools sub menu.
  if (media_router::MediaRouterEnabled(browser()->profile())) {
    SimpleMenuModel* parent_model_for_cast = this;
    if (features::IsChromeRefresh2023()) {
      DCHECK(GetIndexOfCommandId(IDC_SAVE_AND_SHARE_MENU).has_value());
      parent_model_for_cast = static_cast<SimpleMenuModel*>(GetSubmenuModelAt(
          GetIndexOfCommandId(IDC_SAVE_AND_SHARE_MENU).value()));
    }

    DCHECK(parent_model_for_cast->GetIndexOfCommandId(IDC_ROUTE_MEDIA)
               .has_value());
    parent_model_for_cast->RemoveItemAt(
        GetIndexOfCommandId(IDC_ROUTE_MEDIA).value());
  }

  // Remove upstream's clear browsing data. It'll be added into history sub
  // menu at RecentTabsSubMenuModel::Build().
  if (features::IsChromeRefresh2023()) {
    auto index = GetIndexOfCommandId(IDC_CLEAR_BROWSING_DATA);
    CHECK(index);
    RemoveItemAt(*index);

    // Remove upstream's profile menu. "Add new profile" will be added into more
    // tools sub menu.
    index = GetIndexOfCommandId(IDC_PROFILE_MENU_IN_APP_MENU);
    CHECK(index);
    RemoveItemAt(*index);
  } else {
    const auto index =
        more_tools_model->GetIndexOfCommandId(IDC_CLEAR_BROWSING_DATA);
    more_tools_model->RemoveItemAt(*index);
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

  // Remove upstream's about menu. It's moved into help sub menu.
  if (const auto index = GetIndexOfCommandId(IDC_ABOUT)) {
    RemoveItemAt(*index);
  }

  // Remove upstream's distill menu. It's replaced with speedreader.
  if (const auto index = GetIndexOfCommandId(IDC_DISTILL_PAGE)) {
    RemoveItemAt(*index);
  }
}

void BraveAppMenuModel::ExecuteCommand(int id, int event_flags) {
#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
  if (id >= IDC_CONTENT_CONTEXT_IMPORT_IPNS_KEYS_START &&
      id <= IDC_CONTENT_CONTEXT_IMPORT_IPNS_KEYS_END) {
    int ipfs_command = GetSelectedIPFSCommandId(id);
    if (ipfs_command == -1) {
      return;
    }
    auto* submenu = ipns_keys_submenu_models_[ipfs_command].get();
    std::optional<size_t> command_index = submenu->GetIndexOfCommandId(id);
    if (!command_index.has_value()) {
      return;
    }
    auto label = base::UTF16ToUTF8(submenu->GetLabelAt(command_index.value()));
    auto key_name = (command_index.value() > 0) ? label : std::string();
    ExecuteIPFSCommand(ipfs_command, key_name);
    return;
  }
  switch (id) {
    case IDC_APP_MENU_IPFS_SHARE_LOCAL_FILE:
    case IDC_APP_MENU_IPFS_SHARE_LOCAL_FOLDER:
      ExecuteIPFSCommand(id, std::string());
      return;
  }
#endif
  return AppMenuModel::ExecuteCommand(id, event_flags);
}

bool BraveAppMenuModel::IsCommandIdEnabled(int id) const {
#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
  content::BrowserContext* browser_context =
      static_cast<content::BrowserContext*>(browser()->profile());
  if (id >= IDC_CONTENT_CONTEXT_IMPORT_IPNS_KEYS_START &&
      id <= IDC_CONTENT_CONTEXT_IMPORT_IPNS_KEYS_END) {
    if (!IpnsKeysAvailable(browser_context)) {
      return false;
    }
    return true;
  }
  switch (id) {
    case IDC_APP_MENU_IPFS_SHARE_LOCAL_FILE:
    case IDC_APP_MENU_IPFS_SHARE_LOCAL_FOLDER:
    case IDC_APP_MENU_IPFS_PUBLISH_LOCAL_FILE:
    case IDC_APP_MENU_IPFS:
    case IDC_APP_MENU_IPFS_PUBLISH_LOCAL_FOLDER:
    case IDC_APP_MENU_IPFS_OPEN_FILES:
    case IDC_APP_MENU_IPFS_UPDATE_IPNS:
      return ipfs::IsIpfsMenuEnabled(browser()->profile()->GetPrefs()) &&
             IsIpfsServiceLaunched(browser_context);
  }
#endif
  if (id == IDC_EXTENSIONS_SUBMENU_MANAGE_EXTENSIONS) {
    // Always returns true as this command id is only added when it could be
    // used.
    return true;
  }
  return AppMenuModel::IsCommandIdEnabled(id);
}

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
int BraveAppMenuModel::AddIpnsKeysToSubMenu(ui::SimpleMenuModel* submenu,
                                            ipfs::IpnsKeysManager* manager,
                                            int key_command_id) {
  if (!manager) {
    return 0;
  }
  int command_id = key_command_id;

  for (const auto& it : manager->GetKeys()) {
    submenu->AddItem(command_id, base::ASCIIToUTF16(it.first));
    int length = std::ceil(it.second.size() / kKeyTrimRate);
    int size = it.second.size();
    auto key_part = it.second.substr(size - length, length);
    auto key_hash_title = std::string("...") + key_part;
    submenu->SetMinorText(command_id - key_command_id,
                          base::ASCIIToUTF16(key_hash_title));
    command_id++;
  }
  return command_id - key_command_id;
}

void BraveAppMenuModel::ExecuteIPFSCommand(int id, const std::string& key) {
  auto* active_content = browser()->tab_strip_model()->GetActiveWebContents();
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_content);
  if (!helper) {
    return;
  }
  switch (id) {
    case IDC_APP_MENU_IPFS_SHARE_LOCAL_FILE:
    case IDC_APP_MENU_IPFS_PUBLISH_LOCAL_FILE:
      helper->ShowImportDialog(ui::SelectFileDialog::SELECT_OPEN_FILE, key);
      break;
    case IDC_APP_MENU_IPFS_SHARE_LOCAL_FOLDER:
    case IDC_APP_MENU_IPFS_PUBLISH_LOCAL_FOLDER:
      helper->ShowImportDialog(ui::SelectFileDialog::SELECT_EXISTING_FOLDER,
                               key);
      break;
  }
}

int BraveAppMenuModel::GetSelectedIPFSCommandId(int id) const {
  for (const auto& it : ipns_keys_submenu_models_) {
    std::optional<size_t> index = it.second->GetIndexOfCommandId(id);
    if (!index.has_value()) {
      continue;
    }
    return it.first;
  }
  return -1;
}

int BraveAppMenuModel::AddIpfsImportMenuItem(int action_command_id,
                                             int string_id,
                                             int keys_command_id) {
  content::BrowserContext* browser_context =
      static_cast<content::BrowserContext*>(browser()->profile());
  if (!IpnsKeysAvailable(browser_context)) {
    return 0;
  }
  DCHECK(!ipns_keys_submenu_models_.count(action_command_id));
  ipns_keys_submenu_models_[action_command_id] =
      std::make_unique<ui::SimpleMenuModel>(this);
  auto* keys_submenu = ipns_keys_submenu_models_[action_command_id].get();
  DCHECK(keys_submenu);
  auto* keys_manager = GetIpnsKeysManager(browser_context);
  DCHECK(keys_manager);
  auto items_added =
      AddIpnsKeysToSubMenu(keys_submenu, keys_manager, keys_command_id);
  ipns_submenu_model_.AddSubMenuWithStringId(action_command_id, string_id,
                                             keys_submenu);
  return items_added;
}
#endif

size_t BraveAppMenuModel::GetNextIndexOfBraveProductsSection() const {
  std::vector<int> commands_to_check = {IDC_APP_MENU_IPFS,
                                        IDC_SHOW_BRAVE_VPN_PANEL,
                                        IDC_BRAVE_VPN_MENU,
                                        IDC_SHOW_BRAVE_WALLET,
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
  return std::nullopt;
}
