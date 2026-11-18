/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "base/base64.h"
#include "base/rand_util.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "components/signin/public/base/gaia_id_hash.h"
#include "components/sync/service/glue/sync_transport_data_prefs.h"
#include "content/public/browser/browsing_data_remover.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browsing_data_remover_test_util.h"
#include "google_apis/gaia/gaia_id.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

std::string GenerateCacheGUID() {
  // 128 bits of randomness, base64-encoded (the sync cache GUID format).
  constexpr int kGuidBytes = 128 / 8;

  return base::Base64Encode(base::RandBytesAsVector(kGuidBytes));
}

}  // namespace

class BraveBrowsingDataRemoverBrowserTest : public InProcessBrowserTest {
 protected:
  void RemoveAndWait(uint64_t remove_mask) {
    content::BrowsingDataRemover* remover =
        browser()->GetProfile()->GetBrowsingDataRemover();
    content::BrowsingDataRemoverCompletionObserver completion_observer(remover);
    remover->RemoveAndReply(
        base::Time::Min(), base::Time::Max(), remove_mask,
        content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB,
        &completion_observer);
    completion_observer.BlockUntilCompletion();
  }

  void KeepSyncGuidAfterClear(uint64_t remove_mask) {
    const signin::GaiaIdHash gaia_id_hash =
        signin::GaiaIdHash::FromGaiaId(GaiaId("user_gaia_id"));
    PrefService* prefs = browser()->GetProfile()->GetPrefs();

    // Set up sync prefs including the cache GUID.
    const std::string guid = GenerateCacheGUID();
    {
      syncer::SyncTransportDataPrefs sync_prefs(prefs, gaia_id_hash);
      sync_prefs.SetCacheGuid(guid);
      ASSERT_EQ(guid, sync_prefs.GetCacheGuid());
    }

    // Clear cookies/storage.
    RemoveAndWait(remove_mask);

    // Ensure the cache GUID wasn't dropped or changed. Use a fresh instance
    // so we don't rely on any state cached before the removal.
    syncer::SyncTransportDataPrefs sync_prefs_after(prefs, gaia_id_hash);
    EXPECT_EQ(guid, sync_prefs_after.GetCacheGuid());
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
