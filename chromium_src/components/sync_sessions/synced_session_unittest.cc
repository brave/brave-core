/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <components/sync_sessions/synced_session_unittest.cc>

namespace sync_sessions {

// Test that virtual_url_prefix is correctly prepended to virtual_url when
// converting a SerializedNavigationEntry to sync protocol buffer.
// This is used by Brave Containers to encode StoragePartitionConfig info
// in the URL for session persistence and sync.
TEST(SyncedSessionTest, VirtualUrlPrefixIncludedInSyncData) {
  using sessions::SerializedNavigationEntry;
  using sessions::SerializedNavigationEntryTestHelper;

  SerializedNavigationEntry navigation =
      SerializedNavigationEntryTestHelper::CreateNavigationForTest();

  // Set a container URL prefix (simulating Brave Containers)
  const std::string container_prefix =
      "containers+550e8400-e29b-41d4-a716-446655440000:";
  navigation.set_virtual_url_prefix(container_prefix);
  navigation.set_virtual_url(GURL("https://example.com/path?query=1"));

  const sync_pb::TabNavigation sync_data =
      SessionNavigationToSyncData(navigation);

  // The synced virtual_url should include the prefix
  EXPECT_EQ(container_prefix + "https://example.com/path?query=1",
            sync_data.virtual_url());

  // Other fields should remain unchanged
  EXPECT_EQ(navigation.referrer_url().spec(), sync_data.referrer());
  EXPECT_EQ(navigation.title(), base::UTF8ToUTF16(sync_data.title()));
}

// Test that when virtual_url_prefix is empty, virtual_url syncs normally
TEST(SyncedSessionTest, VirtualUrlPrefixEmptyBehavesNormally) {
  using sessions::SerializedNavigationEntry;
  using sessions::SerializedNavigationEntryTestHelper;

  SerializedNavigationEntry navigation =
      SerializedNavigationEntryTestHelper::CreateNavigationForTest();

  // Empty prefix (normal tabs without containers)
  navigation.set_virtual_url_prefix("");
  navigation.set_virtual_url(GURL("https://example.com/"));

  const sync_pb::TabNavigation sync_data =
      SessionNavigationToSyncData(navigation);

  // The synced virtual_url should just be the URL itself
  EXPECT_EQ("https://example.com/", sync_data.virtual_url());
}

// Test that complex URLs with virtual_url_prefix are correctly combined
TEST(SyncedSessionTest, VirtualUrlPrefixWithComplexUrl) {
  using sessions::SerializedNavigationEntry;
  using sessions::SerializedNavigationEntryTestHelper;

  SerializedNavigationEntry navigation =
      SerializedNavigationEntryTestHelper::CreateNavigationForTest();

  const std::string container_prefix =
      "containers+abcd1234-5678-90ef-ghij-klmnopqrstuv:";
  navigation.set_virtual_url_prefix(container_prefix);

  // Complex URL with path, query, and fragment
  const GURL complex_url(
      "https://example.com:8080/path/to/page?query=value&foo=bar#fragment");
  navigation.set_virtual_url(complex_url);

  const sync_pb::TabNavigation sync_data =
      SessionNavigationToSyncData(navigation);

  // Verify the full URL with prefix is synced
  EXPECT_EQ(container_prefix + complex_url.spec(), sync_data.virtual_url());

  // Verify the prefix doesn't interfere with other fields
  EXPECT_EQ(navigation.unique_id(), sync_data.unique_id());
  EXPECT_EQ(navigation.referrer_url().spec(), sync_data.referrer());
}

}  // namespace sync_sessions
