/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <cstddef>

#include "base/android/jni_android.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "chrome/android/chrome_jni_headers/BraveEphemeralStorageUtils_jni.h"
#include "chrome/browser/android/tab_android.h"
#include "content/public/browser/site_instance.h"

namespace ephemeral_storage {

static void JNI_BraveEphemeralStorageUtils_CleanupTLDFirstPartyStorage(
    JNIEnv* env,
    const jni_zero::JavaRef<jobject>& tab_object) {
  CHECK(env);
  // Validate that GetNativeTab returned a valid TabAndroid pointer
  // GetNativeTab handles null JavaRef validation internally
  TabAndroid* tab_android = TabAndroid::GetNativeTab(env, tab_object);
  if (!tab_android) {
    return;
  }

  content::WebContents* web_contents = tab_android->web_contents();
  if (!web_contents) {
    return;
  }

  auto* ephemeral_storage_service =
      EphemeralStorageServiceFactory::GetForContext(
          web_contents->GetBrowserContext());
  if (!ephemeral_storage_service) {
    return;
  }

  ephemeral_storage_service->CleanupTLDFirstPartyStorage(
      web_contents,
      web_contents->GetSiteInstance()->GetStoragePartitionConfig(), true);
}

}  // namespace ephemeral_storage
