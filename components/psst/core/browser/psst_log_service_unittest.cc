// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/core/browser/psst_log_service.h"

#include <optional>
#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace psst {

namespace {

class TestObserver : public PsstLogService::Observer {
 public:
  void OnPsstLogEntryAdded(const mojom::PsstLogEntry& entry) override {
    added_messages.push_back(entry.message);
  }

  void OnPsstLogCleared() override { ++cleared_count; }

  std::vector<std::string> added_messages;
  int cleared_count = 0;
};

}  // namespace

class PsstLogServiceTest : public testing::Test {
 protected:
  PsstLogService service_;
};

TEST_F(PsstLogServiceTest, EntriesAreReturnedInOrder) {
  service_.AddNativeEntry(mojom::PsstLogLevel::kInfo, /*tab_id=*/1,
                          GURL("https://a.example.com/page"), "first");
  service_.AddEntry(mojom::PsstLogSource::kScript, mojom::PsstLogLevel::kWarn,
                    std::nullopt, GURL("https://a.example.com/page"), "second");

  const auto entries = service_.GetEntries();
  ASSERT_EQ(2u, entries.size());

  EXPECT_EQ("first", entries[0]->message);
  EXPECT_EQ(mojom::PsstLogSource::kNative, entries[0]->source);
  EXPECT_EQ(mojom::PsstLogLevel::kInfo, entries[0]->level);
  EXPECT_EQ(1, entries[0]->tab_id);

  EXPECT_EQ("second", entries[1]->message);
  EXPECT_EQ(mojom::PsstLogSource::kScript, entries[1]->source);
  EXPECT_EQ(mojom::PsstLogLevel::kWarn, entries[1]->level);
  EXPECT_EQ(std::nullopt, entries[1]->tab_id);

  EXPECT_LE(entries[0]->timestamp_ms, entries[1]->timestamp_ms);
  EXPECT_GT(entries[0]->timestamp_ms, 0);
}

TEST_F(PsstLogServiceTest, SiteIsReducedToETldPlusOne) {
  service_.AddNativeEntry(mojom::PsstLogLevel::kInfo, /*tab_id=*/1,
                          GURL("https://deep.sub.example.co.uk/a/b?c=d"),
                          "message");
  // No registry: falls back to the host.
  service_.AddNativeEntry(mojom::PsstLogLevel::kInfo, /*tab_id=*/1,
                          GURL("http://127.0.0.1:8080/x"), "message");
  // Invalid URL: no site at all.
  service_.AddNativeEntry(mojom::PsstLogLevel::kInfo, /*tab_id=*/1, GURL(),
                          "message");

  const auto entries = service_.GetEntries();
  ASSERT_EQ(3u, entries.size());
  EXPECT_EQ("example.co.uk", entries[0]->site);
  EXPECT_EQ("127.0.0.1", entries[1]->site);
  EXPECT_EQ("", entries[2]->site);
}

TEST_F(PsstLogServiceTest, DataIsSerializedAsJson) {
  service_.AddEntry(mojom::PsstLogSource::kScript, mojom::PsstLogLevel::kDebug,
                    /*tab_id=*/1, GURL("https://a.test/"), "message",
                    base::DictValue().Set("key", "value"));
  // An empty dictionary is treated as no data.
  service_.AddEntry(mojom::PsstLogSource::kScript, mojom::PsstLogLevel::kDebug,
                    /*tab_id=*/1, GURL("https://a.test/"), "message",
                    base::DictValue());

  const auto entries = service_.GetEntries();
  ASSERT_EQ(2u, entries.size());
  ASSERT_TRUE(entries[0]->data.has_value());
  EXPECT_EQ(base::test::ParseJsonDict(R"({"key":"value"})"),
            base::test::ParseJsonDict(*entries[0]->data));
  EXPECT_FALSE(entries[1]->data.has_value());
}

TEST_F(PsstLogServiceTest, OldestEntriesAreEvictedPastTheCap) {
  constexpr size_t kOverflow = 10;
  for (size_t i = 0; i < PsstLogService::kMaxLogEntries + kOverflow; ++i) {
    service_.AddNativeEntry(mojom::PsstLogLevel::kInfo, /*tab_id=*/1,
                            GURL("https://a.test/"), base::NumberToString(i));
  }

  const auto entries = service_.GetEntries();
  ASSERT_EQ(PsstLogService::kMaxLogEntries, entries.size());
  // The first `kOverflow` entries were dropped.
  EXPECT_EQ(base::NumberToString(kOverflow), entries.front()->message);
  EXPECT_EQ(
      base::NumberToString(PsstLogService::kMaxLogEntries + kOverflow - 1),
      entries.back()->message);
}

TEST_F(PsstLogServiceTest, ClearDiscardsEntries) {
  service_.AddNativeEntry(mojom::PsstLogLevel::kInfo, /*tab_id=*/1,
                          GURL("https://a.test/"), "message");
  ASSERT_EQ(1u, service_.GetEntries().size());

  service_.Clear();
  EXPECT_TRUE(service_.GetEntries().empty());
}

TEST_F(PsstLogServiceTest, ObserversAreNotified) {
  TestObserver observer;
  service_.AddObserver(&observer);

  service_.AddNativeEntry(mojom::PsstLogLevel::kInfo, /*tab_id=*/1,
                          GURL("https://a.test/"), "first");
  service_.AddNativeEntry(mojom::PsstLogLevel::kError, /*tab_id=*/1,
                          GURL("https://a.test/"), "second");
  service_.Clear();

  EXPECT_EQ(std::vector<std::string>({"first", "second"}),
            observer.added_messages);
  EXPECT_EQ(1, observer.cleared_count);

  service_.RemoveObserver(&observer);
  service_.AddNativeEntry(mojom::PsstLogLevel::kInfo, /*tab_id=*/1,
                          GURL("https://a.test/"), "third");
  EXPECT_EQ(2u, observer.added_messages.size());
}

}  // namespace psst
