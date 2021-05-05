/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IPFS_HELPER_H_
#define BRAVE_BROWSER_IPFS_IPFS_HELPER_H_

#include <string>

#include "components/version_info/channel.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace ui {
class SimpleMenuModel;
}  // namespace ui

namespace ipfs {

class IpnsKeysManager;

bool IsIpfsMenuEnabled(content::BrowserContext* browser_context);
IpnsKeysManager* GetIpnsKeysManager(content::BrowserContext* browser_context);
bool IpnsKeysAvailable(content::BrowserContext* browser_context);
int AddIpnsKeysToSubMenu(ui::SimpleMenuModel* submenu,
                         IpnsKeysManager* manager,
                         int key_command_id);
}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_HELPER_H_
