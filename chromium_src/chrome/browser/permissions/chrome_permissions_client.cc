/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/permissions/chrome_permissions_client.h"

#define MaybeCreateMessageUI MaybeCreateMessageUI_ChromiumImpl
#include "src/chrome/browser/permissions/chrome_permissions_client.cc"
#undef MaybeCreateMessageUI

#include <vector>

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
  // Note that requesting_origin has an address in it at this point.
  // But even if we get the original origin without the address, we can't
  // check it against the embedding origin for BRAVE_ETHEREUM and BRAVE_SOLANA
  // here because it can be allowed across origins via the iframe `allow`
  // attribute with the `ethereum` and `solana` feature policy.
  // Without this check we'd fail Chromium's origin check.
  // We instead handle this in brave_wallet_render_frame_observer.cc by not
  // exposing the API which can request permission when the origin is 3p and
  // the feature policy is not allowed explicitly. We ensure that the correct
  // handling is covered via the browser tests:
  // SolanaProviderRendererTest.Iframe3P and
  // JSEthereumProviderBrowserTest.Iframe3P
  if (type == ContentSettingsType::BRAVE_ETHEREUM ||
      type == ContentSettingsType::BRAVE_SOLANA) {
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
  std::vector<raw_ptr<permissions::PermissionRequest, VectorExperimental>>
      requests = prompt->delegate()->Requests();
  if (requests.size() > 0) {
    brave_wallet::mojom::CoinType coin_type =
        brave_wallet::mojom::CoinType::ETH;
    permissions::RequestType request_type = requests[0]->request_type();
    if (request_type == permissions::RequestType::kBraveEthereum ||
        request_type == permissions::RequestType::kBraveSolana) {
      if (request_type == permissions::RequestType::kBraveSolana) {
        coin_type = brave_wallet::mojom::CoinType::SOL;
      }
      auto delegate = std::make_unique<BraveWalletPermissionPrompt::Delegate>(
          std::move(prompt));
      return std::make_unique<BraveWalletPermissionPrompt>(
          web_contents, std::move(delegate), coin_type);
    }
  }

  return MaybeCreateMessageUI_ChromiumImpl(web_contents, type,
                                           std::move(prompt));
}
#endif
