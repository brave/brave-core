/* Copyright (c) 2020 The Brave Authors. All rights reserved.
+ * This Source Code Form is subject to the terms of the Mozilla Public
+ * License, v. 2.0. If a copy of the MPL was not distributed with this file,
+ * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_service_delegate_impl.h"
#include "base/path_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/browser/browser_context.h"

namespace ipfs {

IpfsServiceDelegateImpl::IpfsServiceDelegateImpl(
    content::BrowserContext* context) : IpfsServiceDelegate(context) {
}

IpfsServiceDelegateImpl::~IpfsServiceDelegateImpl() {
}

base::FilePath IpfsServiceDelegateImpl::GetUserDataDir() {
  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  return user_data_dir;
}

bool IpfsServiceDelegateImpl::IsTestingProfile() {
  return Profile::FromBrowserContext(context_)->AsTestingProfile();
}

}  // namespace ipfs
