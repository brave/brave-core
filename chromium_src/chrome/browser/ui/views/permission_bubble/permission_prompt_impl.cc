/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/permission_bubble/permission_prompt_impl.h"

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "chrome/browser/ui/permission_bubble/permission_prompt.h"
#include "components/permissions/request_type.h"

#if BUILDFLAG(BRAVE_WALLET_ENABLED) && !defined(OS_ANDROID) && !defined(OS_IOS)
#include "brave/browser/ui/views/permission_bubble/ethereum_permission_prompt_impl.h"
#endif

#define CreatePermissionPrompt CreatePermissionPrompt_ChromiumImpl
#include "../../../../../../../chrome/browser/ui/views/permission_bubble/permission_prompt_impl.cc"
#undef CreatePermissionPrompt

std::unique_ptr<permissions::PermissionPrompt> CreatePermissionPrompt(
    content::WebContents* web_contents,
    permissions::PermissionPrompt::Delegate* delegate) {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
  if (!browser) {
    DLOG(WARNING) << "Permission prompt suppressed because the WebContents is "
                     "not attached to any Browser window.";
    return nullptr;
  }

#if BUILDFLAG(BRAVE_WALLET_ENABLED) && !defined(OS_ANDROID) && !defined(OS_IOS)
  if (delegate->Requests()[0]->GetRequestType() ==
      permissions::RequestType::kBraveEthereum) {
    return std::make_unique<EthereumPermissionPromptImpl>(browser, web_contents,
                                                          delegate);
  }
#endif

  return CreatePermissionPrompt_ChromiumImpl(web_contents, delegate);
}
