/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_helper.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/keys/ipns_keys_manager.h"
#include "brave/grit/brave_generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/simple_menu_model.h"
#include "url/gurl.h"

namespace {
// For convenience, we show the last part of the key in the context menu item.
// The length of the key is divided to this constant and the last part is taken.
int kKeyTrimRate = 5;

}  // namespace

namespace ipfs {

IpnsKeysManager* GetIpnsKeysManager(content::BrowserContext* browser_context) {
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

int AddIpnsKeysToSubMenu(ui::SimpleMenuModel* submenu,
                         IpnsKeysManager* manager,
                         int key_command_id) {
  auto no_key_title =
      l10n_util::GetStringUTF16(IDS_CONTENT_CONTEXT_IMPORT_WITHOUT_PUBLISHING);
  submenu->AddItem(key_command_id, no_key_title);
  int item_index = 1;
  if (manager) {
    for (const auto& it : manager->GetKeys()) {
      submenu->AddItem(key_command_id + item_index,
                       base::ASCIIToUTF16(it.first));
      int length = std::ceil(it.second.size() / kKeyTrimRate);
      int size = it.second.size();
      auto key_part = it.second.substr(size - length, length);
      auto key_hash_title = std::string("...") + key_part;
      submenu->SetMinorText(item_index, base::ASCIIToUTF16(key_hash_title));
      item_index++;
    }
  }
  return item_index;
}

}  // namespace ipfs
