/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/android/ephemeral_storage_utils.h"

#include <cstddef>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "chrome/android/chrome_jni_headers/BraveEphemeralStorageUtils_jni.h"
#include "chrome/browser/android/tab_android.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents_user_data.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace ephemeral_storage {

static void JNI_BraveEphemeralStorageUtils_CleanupTLDEphemeralStorage(
    JNIEnv* env,
    const jni_zero::JavaRef<jobject>& tab_object) {
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

  ephemeral_storage_service->CleanupTLDEphemeralStorage(
      web_contents,
      web_contents->GetSiteInstance()->GetStoragePartitionConfig(), true);
}

static void JNI_BraveEphemeralStorageUtils_CleanupTLDEphemeralStorageCallback(
    JNIEnv* env,
    const jni_zero::JavaRef<jobjectArray>& tab_array) {
  // Get array length
  size_t len = base::android::SafeGetArrayLength(env, tab_array);

  // Iterate through the array
  for (size_t i = 0; i < len; ++i) {
    auto tab_object = jni_zero::ScopedJavaLocalRef<jobject>::Adopt(
        env, static_cast<jobject>(env->GetObjectArrayElement(
                 tab_array.obj(), base::checked_cast<jsize>(i))));
    if (!tab_object.obj()) {
      continue;
    }

    TabAndroid* tab_android = TabAndroid::GetNativeTab(env, tab_object);
    if (!tab_android) {
      return;
    }

    content::WebContents* web_contents = tab_android->web_contents();
    if (!web_contents) {
      return;
    }

    if (auto* ephemeral_storage_tab_helper =
            ephemeral_storage::EphemeralStorageTabHelper::FromWebContents(
                web_contents);
        ephemeral_storage_tab_helper) {
      ephemeral_storage_tab_helper->EnforceEphemeralStorageClean();
    }
  }
}

void CloseTabsWithTLD(const std::string& etldplusone) {
  if (etldplusone.empty() ||
      !net::registry_controlled_domains::HostHasRegistryControlledDomain(
          etldplusone,
          net::registry_controlled_domains::EXCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    return;
  }

  Java_BraveEphemeralStorageUtils_closeTabsWithTLD(
      base::android::AttachCurrentThread(),
      base::android::ConvertUTF8ToJavaString(
          base::android::AttachCurrentThread(), std::move(etldplusone)));
}

}  // namespace ephemeral_storage
