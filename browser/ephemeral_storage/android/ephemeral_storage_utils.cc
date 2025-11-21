/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/android/ephemeral_storage_utils.h"

#include <cstddef>

#include "base/android/jni_android.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "chrome/android/chrome_jni_headers/BraveEphemeralStorageUtils_jni.h"
#include "chrome/browser/android/tab_android.h"
#include "content/public/browser/site_instance.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"

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

void CloseTabsWithTLD(Profile* current_profile, const std::string& etldplusone) {
  CHECK(current_profile);
  if (etldplusone.empty() ||
      !net::registry_controlled_domains::HostHasRegistryControlledDomain(
          etldplusone,
          net::registry_controlled_domains::EXCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    return;
  }

  std::vector<TabAndroid*> tabs_to_close;
  for (TabModel* model : TabModelList::models()) {
    const size_t tab_count = model->GetTabCount();
    for (size_t index = 0; index < tab_count; index++) {
      auto* tab = model->GetTabAt(index);
      // Do not process tabs from other profiles.
      if (!tab || current_profile != tab->profile()) {
        continue;
      }

      content::WebContents* web_contents = tab->GetContents();
      if (!web_contents) {
        continue;
      }

      const auto tab_tld =
          net::URLToEphemeralStorageDomain(web_contents->GetLastCommittedURL());
      if (tab_tld.empty() || tab_tld != etldplusone) {
        continue;
      }

      if (auto* ephemeral_storage_tab_helper =
              ephemeral_storage::EphemeralStorageTabHelper::FromWebContents(
                  web_contents);
          ephemeral_storage_tab_helper) {
        // Enforce storage cleaning before closing the tab.
        ephemeral_storage_tab_helper->EnforceEphemeralStorageClean();

        tabs_to_close.push_back(tab);
      }
    }
  }

  // Close all collected tabs
  for(TabAndroid* tab : tabs_to_close) {
    if(!tab){
      continue;
    }
    tab->Close();
  }
}

}  // namespace ephemeral_storage
