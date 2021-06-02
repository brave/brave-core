/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_app_menu_model.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"

#if BUILDFLAG(IPFS_ENABLED)
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

#if BUILDFLAG(ENABLE_SIDEBAR)
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "chrome/browser/ui/browser.h"
#include "ui/base/l10n/l10n_util.h"
#endif

namespace {

#if BUILDFLAG(ENABLE_SIDEBAR)

using ShowSidebarOption = sidebar::SidebarService::ShowSidebarOption;

class SidebarMenuModel : public ui::SimpleMenuModel,
                         public ui::SimpleMenuModel::Delegate {
 public:
  explicit SidebarMenuModel(Browser* browser)
      : SimpleMenuModel(nullptr), browser_(browser) {
    set_delegate(this);
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
                 l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_ALWAYS));
    AddCheckItem(IDC_SIDEBAR_SHOW_OPTION_MOUSEOVER,
                 l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_MOUSEOVER));
    AddCheckItem(IDC_SIDEBAR_SHOW_OPTION_ONCLICK,
                 l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_ONCLICK));
    AddCheckItem(IDC_SIDEBAR_SHOW_OPTION_NEVER,
                 l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_NEVER));
  }

  ShowSidebarOption ConvertIDCToSidebarShowOptions(int id) const {
    switch (id) {
      case IDC_SIDEBAR_SHOW_OPTION_ALWAYS:
        return ShowSidebarOption::kShowAlways;
      case IDC_SIDEBAR_SHOW_OPTION_MOUSEOVER:
        return ShowSidebarOption::kShowOnMouseOver;
      case IDC_SIDEBAR_SHOW_OPTION_ONCLICK:
        return ShowSidebarOption::kShowOnClick;
      case IDC_SIDEBAR_SHOW_OPTION_NEVER:
        return ShowSidebarOption::kShowNever;
      default:
        break;
    }
    NOTREACHED();
    return ShowSidebarOption::kShowAlways;
  }

  Browser* browser_ = nullptr;
};

#endif

#if BUILDFLAG(IPFS_ENABLED)
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
  if (!service)
    return nullptr;
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
    AppMenuIconController* app_menu_icon_controller)
    : AppMenuModel(provider, browser, app_menu_icon_controller)
#if BUILDFLAG(IPFS_ENABLED)
      ,
      ipfs_submenu_model_(this)
#endif
{
}

BraveAppMenuModel::~BraveAppMenuModel() = default;

void BraveAppMenuModel::Build() {
  // Insert brave items after build chromium items.
  AppMenuModel::Build();
  InsertBraveMenuItems();
  InsertAlternateProfileItems();
}

void BraveAppMenuModel::InsertBraveMenuItems() {
  // Insert & reorder brave menus based on corresponding commands enable status.
  // If we you want to add/remove from app menu, adjust commands enable status
  // at BraveBrowserCommandController.

  // Step 1. Configure tab & windows section.
  if (IsCommandIdEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE)) {
    InsertItemWithStringIdAt(
        GetIndexOfCommandId(IDC_NEW_WINDOW),
        IDC_NEW_TOR_CONNECTION_FOR_SITE,
        IDS_NEW_TOR_CONNECTION_FOR_SITE);
  }
  if (IsCommandIdEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR)) {
    InsertItemWithStringIdAt(GetIndexOfCommandId(IDC_NEW_INCOGNITO_WINDOW) + 1,
                             IDC_NEW_OFFTHERECORD_WINDOW_TOR,
                             IDS_NEW_OFFTHERECORD_WINDOW_TOR);
  }

  // Step 2. Configure second section that includes history, downloads and
  // bookmark. Then, insert brave items.

  // First, reorder original menus We want to move them in order of bookmark,
  // download and extensions.
  const int bookmark_item_index = GetIndexOfCommandId(IDC_BOOKMARKS_MENU);
  // If bookmark is not used, we don't need to adjust download item.
  if (bookmark_item_index != -1) {
    // Place download menu under bookmark.
    DCHECK(IsCommandIdEnabled(IDC_SHOW_DOWNLOADS));
    RemoveItemAt(GetIndexOfCommandId(IDC_SHOW_DOWNLOADS));
    InsertItemWithStringIdAt(bookmark_item_index,
                             IDC_SHOW_DOWNLOADS,
                             IDS_SHOW_DOWNLOADS);
  }
  // Move extensions menu under download.
  ui::SimpleMenuModel* model = static_cast<ui::SimpleMenuModel*>(
      GetSubmenuModelAt(GetIndexOfCommandId(IDC_MORE_TOOLS_MENU)));
  DCHECK(model);
  // More tools menu adds extensions item always.
  DCHECK_NE(-1, model->GetIndexOfCommandId(IDC_MANAGE_EXTENSIONS));
  model->RemoveItemAt(model->GetIndexOfCommandId(IDC_MANAGE_EXTENSIONS));

  if (IsCommandIdEnabled(IDC_MANAGE_EXTENSIONS)) {
    InsertItemWithStringIdAt(GetIndexOfCommandId(IDC_SHOW_DOWNLOADS) + 1,
                             IDC_MANAGE_EXTENSIONS,
                             IDS_SHOW_EXTENSIONS);
  }

  if (IsCommandIdEnabled(IDC_SHOW_BRAVE_REWARDS)) {
    InsertItemWithStringIdAt(GetIndexOfBraveRewardsItem(),
                             IDC_SHOW_BRAVE_REWARDS,
                             IDS_SHOW_BRAVE_REWARDS);
  }

  // Insert wallet menu after download menu.
  if (IsCommandIdEnabled(IDC_SHOW_BRAVE_WALLET)) {
    InsertItemWithStringIdAt(GetIndexOfCommandId(IDC_SHOW_DOWNLOADS) + 1,
                             IDC_SHOW_BRAVE_WALLET,
                             IDS_SHOW_BRAVE_WALLET);
  }

  // Insert sync menu
  if (IsCommandIdEnabled(IDC_SHOW_BRAVE_SYNC)) {
    InsertItemWithStringIdAt(GetIndexOfBraveSyncItem(),
                             IDC_SHOW_BRAVE_SYNC,
                             IDS_SHOW_BRAVE_SYNC);
  }

#if BUILDFLAG(ENABLE_SIDEBAR)
  if (sidebar::CanUseSidebar(browser()->profile())) {
    sub_menus_.push_back(std::make_unique<SidebarMenuModel>(browser()));
    InsertSubMenuWithStringIdAt(
        GetIndexOfBraveSidebarItem(), IDC_SIDEBAR_SHOW_OPTION_MENU,
        IDS_SIDEBAR_SHOW_OPTION_TITLE, sub_menus_.back().get());
  }
#endif

  // Insert adblock menu at last. Assumed this is always enabled.
  DCHECK(IsCommandIdEnabled(IDC_SHOW_BRAVE_ADBLOCK));
  InsertItemWithStringIdAt(GetIndexOfBraveAdBlockItem(),
                           IDC_SHOW_BRAVE_ADBLOCK,
                           IDS_SHOW_BRAVE_ADBLOCK);

  // Insert webcompat reporter item.
  InsertItemWithStringIdAt(GetIndexOfCommandId(IDC_ABOUT),
                           IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER,
                           IDS_SHOW_BRAVE_WEBCOMPAT_REPORTER);

#if BUILDFLAG(IPFS_ENABLED)
  if (IsCommandIdEnabled(IDC_APP_MENU_IPFS)) {
    int keys_command_index = IDC_CONTENT_CONTEXT_IMPORT_IPNS_KEYS_START;
    keys_command_index += AddIpfsImportMenuItem(
        IDC_APP_MENU_IPFS_IMPORT_LOCAL_FILE,
        IDS_APP_MENU_IPFS_IMPORT_LOCAL_FILE, keys_command_index);
    keys_command_index += AddIpfsImportMenuItem(
        IDC_APP_MENU_IPFS_IMPORT_LOCAL_FOLDER,
        IDS_APP_MENU_IPFS_IMPORT_LOCAL_FOLDER, keys_command_index);
    int index = IsCommandIdEnabled(IDC_SHOW_BRAVE_SYNC)
                    ? GetIndexOfBraveSyncItem() + 1
                    : GetIndexOfBraveAdBlockItem();
    InsertSubMenuWithStringIdAt(index, IDC_APP_MENU_IPFS, IDS_APP_MENU_IPFS,
                                &ipfs_submenu_model_);
    auto& bundle = ui::ResourceBundle::GetSharedInstance();
    const auto& ipfs_logo = *bundle.GetImageSkiaNamed(IDR_BRAVE_IPFS_LOGO);
    ui::ImageModel model = ui::ImageModel::FromImageSkia(ipfs_logo);
    SetIcon(index, model);
  }
#endif
}

void BraveAppMenuModel::ExecuteCommand(int id, int event_flags) {
#if BUILDFLAG(IPFS_ENABLED)
  if (id >= IDC_CONTENT_CONTEXT_IMPORT_IPNS_KEYS_START &&
      id <= IDC_CONTENT_CONTEXT_IMPORT_IPNS_KEYS_END) {
    int ipfs_command = GetSelectedIPFSCommandId(id);
    if (ipfs_command == -1)
      return;
    auto* submenu = ipns_submenu_models_[ipfs_command].get();
    auto command_index = submenu->GetIndexOfCommandId(id);
    if (command_index == -1)
      return;
    auto label = base::UTF16ToUTF8(submenu->GetLabelAt(command_index));
    auto key_name = (command_index > 0) ? label : std::string();
    ExecuteIPFSCommand(ipfs_command, key_name);
    return;
  }
  switch (id) {
    case IDC_APP_MENU_IPFS_IMPORT_LOCAL_FILE:
    case IDC_APP_MENU_IPFS_IMPORT_LOCAL_FOLDER:
      ExecuteIPFSCommand(id, std::string());
      return;
  }
#endif
  return AppMenuModel::ExecuteCommand(id, event_flags);
}

bool BraveAppMenuModel::IsCommandIdEnabled(int id) const {
#if BUILDFLAG(IPFS_ENABLED)
  content::BrowserContext* browser_context =
      static_cast<content::BrowserContext*>(browser()->profile());
  if (id >= IDC_CONTENT_CONTEXT_IMPORT_IPNS_KEYS_START &&
      id <= IDC_CONTENT_CONTEXT_IMPORT_IPNS_KEYS_END) {
    if (!IpnsKeysAvailable(browser_context))
      return false;
    if (ipns_keys_title_item_index_ != -1 &&
        FindCommandIndex(id) == ipns_keys_title_item_index_) {
      return false;
    }
    return true;
  }

  switch (id) {
    case IDC_APP_MENU_IPFS_IMPORT_LOCAL_FILE:
    case IDC_APP_MENU_IPFS:
    case IDC_APP_MENU_IPFS_IMPORT_LOCAL_FOLDER:
      return ipfs::IsIpfsMenuEnabled(browser()->profile()->GetPrefs()) &&
             IsIpfsServiceLaunched(browser_context);
  }
#endif
  return AppMenuModel::IsCommandIdEnabled(id);
}

#if BUILDFLAG(IPFS_ENABLED)
int BraveAppMenuModel::AddIpnsKeysToSubMenu(ui::SimpleMenuModel* submenu,
                                            ipfs::IpnsKeysManager* manager,
                                            int key_command_id) {
  if (!manager)
    return 0;
  int command_id = key_command_id + 1;

  auto no_key_title = l10n_util::GetStringUTF16(IDS_IMPORT_WITHOUT_PUBLISHING);
  submenu->AddItem(command_id++, no_key_title);

  submenu->AddSeparator(ui::NORMAL_SEPARATOR);

  auto ipns_key_title =
      l10n_util::GetStringUTF16(IDS_IMPORT_USING_IPNS_KEYS_TITLE);
  submenu->AddItem(command_id, ipns_key_title);
  ipns_keys_title_item_index_ = command_id - key_command_id;
  command_id++;

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

int BraveAppMenuModel::FindCommandIndex(int command_id) const {
  for (const auto& it : ipns_submenu_models_) {
    int index = it.second->GetIndexOfCommandId(command_id);
    if (index == -1)
      continue;
    return index;
  }
  return -1;
}

void BraveAppMenuModel::ExecuteIPFSCommand(int id, const std::string& key) {
  auto* active_content = browser()->tab_strip_model()->GetActiveWebContents();
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_content);
  if (!helper)
    return;
  switch (id) {
    case IDC_APP_MENU_IPFS_IMPORT_LOCAL_FILE:
      helper->ShowImportDialog(ui::SelectFileDialog::SELECT_OPEN_FILE, key);
      break;
    case IDC_APP_MENU_IPFS_IMPORT_LOCAL_FOLDER:
      helper->ShowImportDialog(ui::SelectFileDialog::SELECT_EXISTING_FOLDER,
                               key);
      break;
  }
}

int BraveAppMenuModel::GetSelectedIPFSCommandId(int id) const {
  for (const auto& it : ipns_submenu_models_) {
    auto index = it.second->GetIndexOfCommandId(id);
    if (index == -1)
      continue;
    return it.first;
  }
  return -1;
}
int BraveAppMenuModel::AddIpfsImportMenuItem(int action_command_id,
                                             int string_id,
                                             int keys_command_id) {
  content::BrowserContext* browser_context =
      static_cast<content::BrowserContext*>(browser()->profile());
  if (IpnsKeysAvailable(browser_context)) {
    DCHECK(!ipns_submenu_models_.count(action_command_id));
    ipns_submenu_models_[action_command_id] =
        std::make_unique<ui::SimpleMenuModel>(this);
    auto* keys_submenu = ipns_submenu_models_[action_command_id].get();
    DCHECK(keys_submenu);
    auto* keys_manager = GetIpnsKeysManager(browser_context);
    DCHECK(keys_manager);
    auto items_added =
        AddIpnsKeysToSubMenu(keys_submenu, keys_manager, keys_command_id);
    ipfs_submenu_model_.AddSubMenuWithStringId(action_command_id, string_id,
                                               keys_submenu);
    return items_added;
  }
  ipfs_submenu_model_.AddItemWithStringId(action_command_id, string_id);
  return 0;
}
#endif
void BraveAppMenuModel::InsertAlternateProfileItems() {
  // Insert Open Guest Window and Create New Profile items just above
  // the zoom item unless these items are disabled.

  const int zoom_index = GetIndexOfCommandId(IDC_ZOOM_MENU);
  const int index = zoom_index - 1;

  // Open Guest Window
  if (IsCommandIdEnabled(IDC_OPEN_GUEST_PROFILE)) {
    InsertItemWithStringIdAt(index, IDC_OPEN_GUEST_PROFILE,
                             IDS_OPEN_GUEST_PROFILE);
  }

  // Create New Profile
  if (IsCommandIdEnabled(IDC_ADD_NEW_PROFILE)) {
    InsertItemWithStringIdAt(index, IDC_ADD_NEW_PROFILE, IDS_ADD_NEW_PROFILE);
  }

  if (zoom_index != GetIndexOfCommandId(IDC_ZOOM_MENU))
    InsertSeparatorAt(index, ui::NORMAL_SEPARATOR);
}

int BraveAppMenuModel::GetIndexOfBraveAdBlockItem() const {
  // Insert as a last item in second section.
  int adblock_item_index = -1;

#if BUILDFLAG(ENABLE_SIDEBAR)
  adblock_item_index = GetIndexOfCommandId(IDC_SIDEBAR_SHOW_OPTION_MENU);
  if (adblock_item_index != -1)
    return adblock_item_index + 1;
#endif

  adblock_item_index = GetIndexOfCommandId(IDC_SHOW_BRAVE_SYNC);
  if (adblock_item_index != -1)
    return adblock_item_index + 1;

  adblock_item_index = GetIndexOfCommandId(IDC_MANAGE_EXTENSIONS);
  if (adblock_item_index != -1)
    return adblock_item_index + 1;

  adblock_item_index = GetIndexOfCommandId(IDC_SHOW_BRAVE_WALLET);
  if (adblock_item_index != -1)
    return adblock_item_index + 1;

  adblock_item_index = GetIndexOfCommandId(IDC_SHOW_DOWNLOADS);
  DCHECK_NE(-1, adblock_item_index) << "No download item";
  return adblock_item_index + 1;
}

int BraveAppMenuModel::GetIndexOfBraveRewardsItem() const {
  // Insert rewards menu at first of this section. If history menu is not
  // available, check below items.
  int rewards_index = -1;
  rewards_index = GetIndexOfCommandId(IDC_RECENT_TABS_MENU);
  if (rewards_index != -1)
    return rewards_index;

  rewards_index = GetIndexOfCommandId(IDC_BOOKMARKS_MENU);
  if (rewards_index != -1)
    return rewards_index;

  rewards_index = GetIndexOfCommandId(IDC_SHOW_DOWNLOADS);
  DCHECK_NE(-1, rewards_index) << "No download item";
  return rewards_index;
}

int BraveAppMenuModel::GetIndexOfBraveSyncItem() const {
  // Insert sync menu under extensions menu. If extensions menu is not
  // available, check above items.
  int sync_index = -1;
  sync_index = GetIndexOfCommandId(IDC_MANAGE_EXTENSIONS);
  if (sync_index != -1)
    return sync_index + 1;

  sync_index = GetIndexOfCommandId(IDC_SHOW_BRAVE_WALLET);
  if (sync_index != -1)
    return sync_index + 1;

  sync_index = GetIndexOfCommandId(IDC_SHOW_DOWNLOADS);
  DCHECK_NE(-1, sync_index) << "No download item";
  return sync_index + 1;
}

#if BUILDFLAG(ENABLE_SIDEBAR)
int BraveAppMenuModel::GetIndexOfBraveSidebarItem() const {
  // Insert as a last item in second section.
  int sidebar_item_index = -1;
  sidebar_item_index = GetIndexOfCommandId(IDC_SHOW_BRAVE_SYNC);
  if (sidebar_item_index != -1)
    return sidebar_item_index + 1;

  sidebar_item_index = GetIndexOfCommandId(IDC_MANAGE_EXTENSIONS);
  if (sidebar_item_index != -1)
    return sidebar_item_index + 1;

  sidebar_item_index = GetIndexOfCommandId(IDC_SHOW_BRAVE_WALLET);
  if (sidebar_item_index != -1)
    return sidebar_item_index + 1;

  sidebar_item_index = GetIndexOfCommandId(IDC_SHOW_DOWNLOADS);
  DCHECK_NE(-1, sidebar_item_index) << "No download item";
  return sidebar_item_index + 1;
}
#endif
