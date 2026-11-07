/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_access_result.h"
#include "net/cookies/cookie_change_dispatcher.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_deletion_info.h"
#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/cookie_monster.h"
#include "net/cookies/cookie_options.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace net {
namespace {

class CookieMonsterEphemeralTest : public testing::Test {
 protected:
  CookieMonsterEphemeralTest() : cm_(nullptr, nullptr) {}

  CookieOptions MakePersistentOptions() {
    return CookieOptions::MakeAllInclusive();
  }

  CookieOptions MakeEphemeralOptions(const GURL& top_frame_url) {
    CookieOptions options = MakePersistentOptions();
    options.set_should_use_ephemeral_storage(true);
    options.set_top_frame_origin(url::Origin::Create(top_frame_url));
    return options;
  }

  bool SetCookie(const GURL& url,
                 const std::string& cookie_line,
                 const CookieOptions& options) {
    base::RunLoop loop;
    bool included = false;
    cm_.SetCanonicalCookieAsync(
        CanonicalCookie::CreateForTesting(url, cookie_line, base::Time::Now(),
                                          CookieSourceType::kOther),
        url, options,
        base::BindLambdaForTesting([&](CookieAccessResult result) {
          included = result.status.IsInclude();
          loop.Quit();
        }),
        std::nullopt);
    loop.Run();
    return included;
  }

  uint32_t DeleteEphemeralDomain(const std::string& domain) {
    CookieDeletionInfo delete_info;
    delete_info.ephemeral_storage_domain = domain;
    base::RunLoop loop;
    uint32_t deleted = 0;
    cm_.DeleteAllMatchingInfoAsync(
        std::move(delete_info),
        base::BindLambdaForTesting([&](uint32_t num_deleted) {
          deleted = num_deleted;
          loop.Quit();
        }));
    loop.Run();
    return deleted;
  }

  std::unique_ptr<CookieChangeSubscription> SubscribeUrl(
      const GURL& url,
      std::vector<CookieChangeInfo>* changes) {
    return cm_.GetChangeDispatcher().AddCallbackForUrl(
        url, /*cookie_partition_key=*/std::nullopt,
        base::BindLambdaForTesting(
            [changes](const CookieChangeInfo& change) {
              changes->push_back(change);
            }));
  }

  base::test::TaskEnvironment task_environment_;
  CookieMonster cm_;
};

TEST_F(CookieMonsterEphemeralTest,
       UrlSubscriptionSeesEphemeralWriteCreatedAfterSubscribe) {
  const GURL cookie_url("https://x.com/");
  const GURL top_frame_a("https://a.com/");

  std::vector<CookieChangeInfo> changes;
  auto subscription = SubscribeUrl(cookie_url, &changes);
  ASSERT_TRUE(subscription);

  ASSERT_TRUE(SetCookie(cookie_url, "n=ephemeral-a",
                        MakeEphemeralOptions(top_frame_a)));
  ASSERT_EQ(1u, changes.size());
  EXPECT_EQ("n", changes[0].cookie.Name());
  EXPECT_EQ("ephemeral-a", changes[0].cookie.Value());
  EXPECT_EQ(CookieChangeCause::INSERTED, changes[0].cause);
}

TEST_F(CookieMonsterEphemeralTest,
       UrlSubscriptionSeesSecondEphemeralStoreCreatedLater) {
  const GURL cookie_url("https://x.com/");
  const GURL top_frame_a("https://a.com/");
  const GURL top_frame_b("https://b.com/");

  std::vector<CookieChangeInfo> changes;
  auto subscription = SubscribeUrl(cookie_url, &changes);
  ASSERT_TRUE(subscription);

  ASSERT_TRUE(SetCookie(cookie_url, "n=ephemeral-a",
                        MakeEphemeralOptions(top_frame_a)));
  ASSERT_TRUE(SetCookie(cookie_url, "n=ephemeral-b",
                        MakeEphemeralOptions(top_frame_b)));
  ASSERT_EQ(2u, changes.size());
  EXPECT_EQ("ephemeral-a", changes[0].cookie.Value());
  EXPECT_EQ("ephemeral-b", changes[1].cookie.Value());
}

TEST_F(CookieMonsterEphemeralTest, PersistentWriteNotifiesOnce) {
  const GURL cookie_url("https://x.com/");

  std::vector<CookieChangeInfo> changes;
  auto subscription = SubscribeUrl(cookie_url, &changes);
  ASSERT_TRUE(subscription);

  ASSERT_TRUE(SetCookie(cookie_url, "n=persistent", MakePersistentOptions()));
  EXPECT_EQ(1u, changes.size());
  EXPECT_EQ("persistent", changes[0].cookie.Value());
  EXPECT_EQ(CookieChangeCause::INSERTED, changes[0].cause);
}

TEST_F(CookieMonsterEphemeralTest,
       ErasingEphemeralDomainNotifiesUrlSubscribers) {
  const GURL cookie_url("https://x.com/");
  const GURL top_frame_a("https://a.com/");

  std::vector<CookieChangeInfo> changes;
  auto subscription = SubscribeUrl(cookie_url, &changes);
  ASSERT_TRUE(subscription);

  ASSERT_TRUE(SetCookie(cookie_url, "n=ephemeral-a",
                        MakeEphemeralOptions(top_frame_a)));
  changes.clear();

  EXPECT_EQ(1u, DeleteEphemeralDomain("a.com"));
  ASSERT_EQ(1u, changes.size());
  EXPECT_EQ("ephemeral-a", changes[0].cookie.Value());
  EXPECT_TRUE(CookieChangeCauseIsDeletion(changes[0].cause));
}

TEST_F(CookieMonsterEphemeralTest,
       GlobalListenerDoesNotSeeEphemeralWrites) {
  const GURL cookie_url("https://x.com/");
  const GURL top_frame_a("https://a.com/");

  std::vector<CookieChangeInfo> changes;
  auto subscription = cm_.GetChangeDispatcher().AddCallbackForAllChanges(
      base::BindLambdaForTesting([&](const CookieChangeInfo& change) {
        changes.push_back(change);
      }));
  ASSERT_TRUE(subscription);

  ASSERT_TRUE(SetCookie(cookie_url, "n=ephemeral-a",
                        MakeEphemeralOptions(top_frame_a)));
  EXPECT_TRUE(changes.empty());

  ASSERT_TRUE(SetCookie(cookie_url, "n=persistent", MakePersistentOptions()));
  ASSERT_EQ(1u, changes.size());
  EXPECT_EQ("persistent", changes[0].cookie.Value());
}

}  // namespace
}  // namespace net
