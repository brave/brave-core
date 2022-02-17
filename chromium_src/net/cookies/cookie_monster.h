/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_COOKIES_COOKIE_MONSTER_H_
#define BRAVE_CHROMIUM_SRC_NET_COOKIES_COOKIE_MONSTER_H_

#define CookieMonster ChromiumCookieMonster
#include "src/net/cookies/cookie_monster.h"
#undef CookieMonster

namespace net {

class NET_EXPORT CookieMonster : public ChromiumCookieMonster {
 public:
  // These constructors and destructors must be kept in sync with those in
  // Chromium's CookieMonster.
  CookieMonster(scoped_refptr<PersistentCookieStore> store,
                NetLog* net_log,
                bool first_party_sets_enabled);
  CookieMonster(scoped_refptr<PersistentCookieStore> store,
                base::TimeDelta last_access_threshold,
                NetLog* net_log,
                bool first_party_sets_enabled);
  ~CookieMonster() override;

  // CookieStore implementation.
  //
  // This only includes methods that needs special behavior to deal with
  // our collection of ephemeral monsters.
  void DeleteCanonicalCookieAsync(const CanonicalCookie& cookie,
                                  DeleteCallback callback) override;
  void DeleteAllCreatedInTimeRangeAsync(
      const CookieDeletionInfo::TimeRange& creation_range,
      DeleteCallback callback) override;
  void DeleteAllMatchingInfoAsync(CookieDeletionInfo delete_info,
                                  DeleteCallback callback) override;
  void DeleteSessionCookiesAsync(DeleteCallback) override;
  void SetCookieableSchemes(const std::vector<std::string>& schemes,
                            SetCookieableSchemesCallback callback) override;

  void SetCanonicalCookieAsync(std::unique_ptr<CanonicalCookie> cookie,
                               const GURL& source_url,
                               const CookieOptions& options,
                               SetCookiesCallback callback) override;
  void GetCookieListWithOptionsAsync(
      const GURL& url,
      const CookieOptions& options,
      const CookiePartitionKeyCollection& cookie_partition_key_collection,
      GetCookieListCallback callback) override;

 private:
  NetLogWithSource net_log_;
  std::map<std::string, std::unique_ptr<ChromiumCookieMonster>>
      ephemeral_cookie_stores_;
  ChromiumCookieMonster* GetOrCreateEphemeralCookieStoreForTopFrameURL(
      const GURL& top_frame_url);
};

}  // namespace net

#endif  // BRAVE_CHROMIUM_SRC_NET_COOKIES_COOKIE_MONSTER_H_
