/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/permission_bubble/permission_prompt.h"
#include "components/permissions/request_type.h"

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
#include "brave/browser/ui/views/permission_bubble/brave_wallet_permission_prompt_impl.h"
#endif

#define CreatePermissionPrompt CreatePermissionPrompt_ChromiumImpl
#include "src/chrome/browser/ui/views/permissions/permission_prompt_factory.cc"
#undef CreatePermissionPrompt

std::unique_ptr<permissions::PermissionPrompt> CreatePermissionPrompt(
    content::WebContents* web_contents,
    permissions::PermissionPrompt::Delegate* delegate) {
  Browser* browser = chrome::FindBrowserWithTab(web_contents);
  if (!browser) {
    DLOG(WARNING) << "Permission prompt suppressed because the WebContents is "
                     "not attached to any Browser window.";
    return nullptr;
  }

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  if (delegate->Requests()[0]->request_type() ==
          permissions::RequestType::kBraveEthereum ||
      delegate->Requests()[0]->request_type() ==
          permissions::RequestType::kBraveSolana) {
    return std::make_unique<BraveWalletPermissionPromptImpl>(
        browser, web_contents, *delegate);
  }
#endif

  return CreatePermissionPrompt_ChromiumImpl(web_contents, delegate);
}
