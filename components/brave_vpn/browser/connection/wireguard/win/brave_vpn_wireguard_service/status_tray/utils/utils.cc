/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/utils/utils.h"

#include <windows.h>  // Should be before shellapi.h

#include <shellapi.h>
#include <memory>

#include "base/logging.h"
#include "ui/gfx/icon_util.h"
#include "ui/gfx/image/image_family.h"
#include "ui/gfx/image/image_skia.h"

namespace brave_vpn {

namespace {

std::unique_ptr<gfx::ImageFamily> GetAppIconImageFamily(int icon_id) {
  // Get the icon from the current module.
  HMODULE module = GetModuleHandle(nullptr);
  DCHECK(module);
  return IconUtil::CreateImageFamilyFromIconResource(module, icon_id);
}
}  // namespace

gfx::ImageSkia GetIconFromResources(int icon_id, const gfx::Size& size) {
  std::unique_ptr<gfx::ImageFamily> family = GetAppIconImageFamily(icon_id);
  DCHECK(family);
  if (!family) {
    return gfx::ImageSkia();
  }

  return family->CreateExact(size).AsImageSkia();
}

void OpenURLInBrowser(const char* url) {
  if (reinterpret_cast<ULONG_PTR>(
          ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL)) <= 32) {
    VLOG(1) << "Failed to open url in browser:" << url;
    return;
  }
}

}  // namespace brave_vpn
