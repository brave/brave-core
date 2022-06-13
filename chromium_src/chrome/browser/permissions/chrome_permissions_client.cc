/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/permissions/chrome_permissions_client.h"

#define MaybeCreateMessageUI MaybeCreateMessageUI_ChromiumImpl
#include "src/chrome/browser/permissions/chrome_permissions_client.cc"
#undef MaybeCreateMessageUI

#include <vector>

#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "build/build_config.h"
#include "components/permissions/permission_request.h"
#include "components/permissions/request_type.h"
#include "url/origin.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/permissions/brave_wallet_permission_prompt_android.h"
#include "components/permissions/android/permission_prompt/permission_prompt_android.h"
#endif

bool ChromePermissionsClient::BraveCanBypassEmbeddingOriginCheck(
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    ContentSettingsType type) {
  // Since requesting_origin has been overwritten by us to add address info,
  // it will fail Chromium's origin check because requesting_origin is now
  // different from the embedding_origin. To address this, we get the original
  // requesting origin back and use it to check with embedding_origin instead,
  // and let it bypass the origin check from Chromium when the original
  // requesting_origin & embedding_origin are the same.
  url::Origin original_requesting_origin;
  if ((type == ContentSettingsType::BRAVE_ETHEREUM ||
       type == ContentSettingsType::BRAVE_SOLANA) &&
      brave_wallet::ParseRequestingOriginFromSubRequest(
          permissions::ContentSettingsTypeToRequestType(type),
          url::Origin::Create(requesting_origin), &original_requesting_origin,
          nullptr) &&
      original_requesting_origin == url::Origin::Create(embedding_origin)) {
    return true;
  }

  return CanBypassEmbeddingOriginCheck(requesting_origin, embedding_origin);
}

#if BUILDFLAG(IS_ANDROID)
std::unique_ptr<ChromePermissionsClient::PermissionMessageDelegate>
ChromePermissionsClient::MaybeCreateMessageUI(
    content::WebContents* web_contents,
    ContentSettingsType type,
    base::WeakPtr<permissions::PermissionPromptAndroid> prompt) {
  std::vector<permissions::PermissionRequest*> requests =
      prompt->delegate()->Requests();
  if (requests.size() != 0 &&
      requests[0]->request_type() == permissions::RequestType::kBraveEthereum) {
    auto delegate = std::make_unique<BraveWalletPermissionPrompt::Delegate>(
        std::move(prompt));
    return std::make_unique<BraveWalletPermissionPrompt>(web_contents,
                                                         std::move(delegate));
  }

  return MaybeCreateMessageUI_ChromiumImpl(web_contents, type,
                                           std::move(prompt));
}
#endif
