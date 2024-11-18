/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <stddef.h>

#include "base/base64.h"
#include "base/rand_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/sync/service/glue/sync_transport_data_prefs.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browsing_data_remover_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

std::string GenerateCacheGUID() {
  // Generate a GUID with 128 bits of randomness.
  constexpr int kGuidBytes = 128 / 8;

  return base::Base64Encode(base::RandBytesAsVector(kGuidBytes));
}

}  // namespace

class BraveBrowsingDataRemoverBrowserTest : public InProcessBrowserTest {
 public:
  BraveBrowsingDataRemoverBrowserTest() = default;

 protected:
  void RemoveAndWait(uint64_t remove_mask) {
    content::BrowsingDataRemover* remover =
        browser()->profile()->GetBrowsingDataRemover();
    content::BrowsingDataRemoverCompletionObserver completion_observer(remover);
    remover->RemoveAndReply(
        base::Time(), base::Time::Max(), remove_mask,
        content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB,
        &completion_observer);
    completion_observer.BlockUntilCompletion();
  }

  void KeepSyncGuidAfterClear(uint64_t remove_mask) {
    // Setup sync prefs including cache_id
    signin::GaiaIdHash gaia_id_hash =
        signin::GaiaIdHash::FromGaiaId("user_gaia_id");
    syncer::SyncTransportDataPrefs sync_transport_data_prefs(
        browser()->profile()->GetPrefs(), gaia_id_hash);
    sync_transport_data_prefs.SetCacheGuid(GenerateCacheGUID());
    EXPECT_FALSE(sync_transport_data_prefs.GetCacheGuid().empty());

    // Clear cookies/storage
    RemoveAndWait(remove_mask);

    // Ensure cache guid wasn't dropped
    EXPECT_FALSE(sync_transport_data_prefs.GetCacheGuid().empty());
  }
};

IN_PROC_BROWSER_TEST_F(BraveBrowsingDataRemoverBrowserTest,
                       KeepSyncGuidAfterClearCookies) {
  KeepSyncGuidAfterClear(content::BrowsingDataRemover::DATA_TYPE_COOKIES);
}

IN_PROC_BROWSER_TEST_F(BraveBrowsingDataRemoverBrowserTest,
                       KeepSyncGuidAfterClearOnStoragePartition) {
  KeepSyncGuidAfterClear(
      content::BrowsingDataRemover::DATA_TYPE_ON_STORAGE_PARTITION);
}
