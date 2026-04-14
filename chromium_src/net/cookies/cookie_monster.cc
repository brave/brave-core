/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <net/cookies/cookie_monster.cc>

namespace net {

CookieMonster::CookieMonster(scoped_refptr<PersistentCookieStore> store,
                             NetLog* net_log,
                             std::unique_ptr<PrefDelegate> pref_delegate)
    : chromium_impl::CookieMonster(store, net_log, std::move(pref_delegate)),
      net_log_(
          NetLogWithSource::Make(net_log, NetLogSourceType::COOKIE_STORE)) {}

CookieMonster::CookieMonster(scoped_refptr<PersistentCookieStore> store,
                             base::TimeDelta last_access_threshold,
                             NetLog* net_log,
                             std::unique_ptr<PrefDelegate> pref_delegate)
    : chromium_impl::CookieMonster(store,
                                   last_access_threshold,
                                   net_log,
                                   std::move(pref_delegate)),
      net_log_(
          NetLogWithSource::Make(net_log, NetLogSourceType::COOKIE_STORE)) {}

CookieMonster::~CookieMonster() {}

chromium_impl::CookieMonster*
CookieMonster::GetOrCreateEphemeralCookieStoreForTopFrameURL(
    const GURL& top_frame_url) {
  std::string domain = URLToEphemeralStorageDomain(top_frame_url);
  auto it = ephemeral_cookie_stores_.find(domain);
  if (it != ephemeral_cookie_stores_.end())
    return it->second.get();

  return ephemeral_cookie_stores_
      .emplace(domain, new chromium_impl::CookieMonster(nullptr /* store */,
                                                        net_log_.net_log()))
      .first->second.get();
}

void CookieMonster::DeleteCanonicalCookieAsync(const CanonicalCookie& cookie,
                                               DeleteCallback callback) {
  for (auto& it : ephemeral_cookie_stores_) {
    it.second->DeleteCanonicalCookieAsync(cookie, DeleteCallback());
  }
  chromium_impl::CookieMonster::DeleteCanonicalCookieAsync(cookie,
                                                           std::move(callback));
}

void CookieMonster::DeleteAllCreatedInTimeRangeAsync(
    const CookieDeletionInfo::TimeRange& creation_range,
    DeleteCallback callback) {
  for (auto& it : ephemeral_cookie_stores_) {
    it.second->DeleteAllCreatedInTimeRangeAsync(creation_range,
                                                DeleteCallback());
  }
  chromium_impl::CookieMonster::DeleteAllCreatedInTimeRangeAsync(
      creation_range, std::move(callback));
}

void CookieMonster::DeleteAllMatchingInfoAsync(CookieDeletionInfo delete_info,
                                               DeleteCallback callback) {
  if (delete_info.ephemeral_storage_domain.has_value()) {
    ephemeral_cookie_stores_.erase(*delete_info.ephemeral_storage_domain);
    std::move(callback).Run(0);
    return;
  }

  for (auto& it : ephemeral_cookie_stores_) {
    it.second->DeleteAllMatchingInfoAsync(delete_info, DeleteCallback());
  }
  chromium_impl::CookieMonster::DeleteAllMatchingInfoAsync(delete_info,
                                                           std::move(callback));
}

void CookieMonster::DeleteSessionCookiesAsync(DeleteCallback callback) {
  for (auto& it : ephemeral_cookie_stores_) {
    it.second->DeleteSessionCookiesAsync(DeleteCallback());
  }
  chromium_impl::CookieMonster::DeleteSessionCookiesAsync(std::move(callback));
}

void CookieMonster::SetCookieableSchemes(
    std::vector<std::string> schemes,
    SetCookieableSchemesCallback callback) {
  for (auto& it : ephemeral_cookie_stores_) {
    it.second->SetCookieableSchemes(schemes, SetCookieableSchemesCallback());
  }
  chromium_impl::CookieMonster::SetCookieableSchemes(std::move(schemes),
                                                     std::move(callback));
}

void CookieMonster::SetCanonicalCookieAsync(
    std::unique_ptr<CanonicalCookie> cookie,
    const GURL& source_url,
    const CookieOptions& options,
    SetCookiesCallback callback,
    std::optional<CookieAccessResult> cookie_access_result) {
  if (options.should_use_ephemeral_storage()) {
    if (!options.top_frame_origin()) {
      // Shouldn't happen, but don't do anything in this case.
      net::CookieInclusionStatus cookie_inclusion_status;
      cookie_inclusion_status.AddExclusionReason(
          net::CookieInclusionStatus::ExclusionReason::EXCLUDE_UNKNOWN_ERROR);

      MaybeRunCookieCallback(std::move(callback),
                             CookieAccessResult(cookie_inclusion_status));
      return;
    }
    chromium_impl::CookieMonster* ephemeral_monster =
        GetOrCreateEphemeralCookieStoreForTopFrameURL(
            options.top_frame_origin()->GetURL());
    ephemeral_monster->SetCanonicalCookieAsync(std::move(cookie), source_url,
                                               options, std::move(callback),
                                               std::move(cookie_access_result));
    return;
  }

  chromium_impl::CookieMonster::SetCanonicalCookieAsync(
      std::move(cookie), source_url, options, std::move(callback),
      std::move(cookie_access_result));
}

void CookieMonster::GetCookieListWithOptionsAsync(
    const GURL& url,
    const CookieOptions& options,
    const CookiePartitionKeyCollection& cookie_partition_key_collection,
    GetCookieListCallback callback) {
  if (options.should_use_ephemeral_storage()) {
    if (!options.top_frame_origin()) {
      // Shouldn't happen, but don't do anything in this case.
      MaybeRunCookieCallback(std::move(callback), CookieAccessResultList(),
                             CookieAccessResultList());
      return;
    }
    chromium_impl::CookieMonster* ephemeral_monster =
        GetOrCreateEphemeralCookieStoreForTopFrameURL(
            options.top_frame_origin()->GetURL());
    ephemeral_monster->GetCookieListWithOptionsAsync(
        url, options, cookie_partition_key_collection, std::move(callback));
    return;
  }

  chromium_impl::CookieMonster::GetCookieListWithOptionsAsync(
      url, options, cookie_partition_key_collection, std::move(callback));
}

}  // namespace net
