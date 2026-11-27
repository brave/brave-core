/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/logging.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "chrome/browser/ui/permission_bubble/permission_prompt.h"
#include "components/permissions/request_type.h"

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/browser/ui/views/permission_bubble/brave_wallet_permission_prompt_impl.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#endif

namespace {

bool IsBraveWalletRequest(permissions::PermissionPrompt::Delegate* delegate) {
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  const auto request_type = delegate->Requests()[0]->request_type();
  return request_type == permissions::RequestType::kBraveEthereum ||
         request_type == permissions::RequestType::kBraveSolana ||
         request_type == permissions::RequestType::kBraveCardano ||
         request_type == permissions::RequestType::kBravePolkadot;
#else
  return false;
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET)
}

std::unique_ptr<permissions::PermissionPrompt>
CreateBraveWalletPermissionPrompt(
    content::WebContents* web_contents,
    permissions::PermissionPrompt::Delegate* delegate) {
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  if (!GlobalBrowserCollection::GetInstance()->FindBrowserWithTab(
          web_contents)) {
    DLOG(WARNING) << "Permission prompt suppressed because the WebContents is "
                     "not attached to any Browser window.";
    return nullptr;
  }
  return std::make_unique<BraveWalletPermissionPromptImpl>(web_contents,
                                                           *delegate);
#else
  return nullptr;
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET)
}

}  // namespace

#include <chrome/browser/ui/views/permissions/permission_prompt_factory.cc>
