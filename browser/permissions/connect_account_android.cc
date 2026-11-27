/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/check.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "chrome/android/chrome_jni_headers/ConnectAccountFragment_jni.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"
#include "url/origin.h"

static bool JNI_ConnectAccountFragment_ConnectAccount(
    JNIEnv* env,
    const base::android::JavaRef<jstring>& java_account_address,
    jint account_id_coin,
    const base::android::JavaRef<jstring>& java_origin_spec,
    const base::android::JavaRef<jobject>& java_web_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(java_web_contents);

  if (web_contents == nullptr) {
    return false;
  }

  std::string account_address =
      base::android::ConvertJavaStringToUTF8(java_account_address);

  // ConnectAccountFragment names the primary main frame's origin, and so do its
  // connected accounts list and Disconnect. Grant to that same frame: using the
  // focused frame would let a cross-origin subframe holding focus receive a
  // durable permission the user was never shown and cannot revoke from that
  // screen.
  content::RenderFrameHost* rfh = web_contents->GetPrimaryMainFrame();
  if (!rfh) {
    return false;
  }
  const url::Origin& origin = rfh->GetLastCommittedOrigin();

  // Tapping Connect is consent only for the site the panel shows. Refuse if the
  // page committed a different origin since the panel last read it.
  if (!origin.IsSameOriginWith(
          GURL(base::android::ConvertJavaStringToUTF8(java_origin_spec)))) {
    return false;
  }

  brave_wallet::mojom::CoinType coin =
      static_cast<brave_wallet::mojom::CoinType>(account_id_coin);
  CHECK(brave_wallet::mojom::IsKnownEnumValue(coin));

  auto request_type = brave_wallet::CoinTypeToPermissionRequestType(coin);
  auto permission = brave_wallet::CoinTypeToPermissionType(coin);

  if (!request_type || !permission) {
    return false;
  }

  if (permissions::BraveWalletPermissionContext::HasRequestsInProgress(
          rfh, *request_type)) {
    return false;
  }

  content::BrowserContext* browser_context = web_contents->GetBrowserContext();

  // Tapping Connect in the panel is the consent, so the grant is written here
  // directly. IsPermissionDenied must run before AddPermission: on a blocked
  // site HasPermission succeeds with has_permission == false, which would make
  // AddPermission write an allow rule over the block.
  if (permissions::BraveWalletPermissionContext::IsPermissionDenied(
          *permission, browser_context, origin) ||
      !permissions::BraveWalletPermissionContext::AddPermission(
          *permission, browser_context, origin, account_address)) {
    return false;
  }

  return true;
}

DEFINE_JNI(ConnectAccountFragment)
