/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <net/cookies/cookie_monster.cc>

#include <cstdint>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "net/base/url_util.h"
#include "net/cookies/cookie_monster_change_dispatcher.h"

namespace net {

CookieMonster::CookieMonster(scoped_refptr<PersistentCookieStore> store,
                             NetLog* net_log,
                             std::unique_ptr<PrefDelegate> pref_delegate)
    : CookieMonster_ChromiumImpl(store, net_log, std::move(pref_delegate)),
      net_log_(
          NetLogWithSource::Make(net_log, NetLogSourceType::COOKIE_STORE)) {}

CookieMonster::CookieMonster(base::PassKey<CookieMonster_ChromiumImpl> key,
                             scoped_refptr<PersistentCookieStore> store,
                             base::TimeDelta last_access_threshold,
                             NetLog* net_log,
                             std::unique_ptr<PrefDelegate> pref_delegate)
    : CookieMonster_ChromiumImpl(key,
                                 store,
                                 last_access_threshold,
                                 net_log,
                                 std::move(pref_delegate)),
      net_log_(
          NetLogWithSource::Make(net_log, NetLogSourceType::COOKIE_STORE)) {}

CookieMonster::~CookieMonster() {}

CookieMonster_ChromiumImpl*
CookieMonster::GetOrCreateEphemeralCookieStoreForTopFrameURL(
    const GURL& top_frame_url) {
  std::string domain = URLToEphemeralStorageDomain(top_frame_url);
  auto it = ephemeral_cookie_stores_.find(domain);
  if (it != ephemeral_cookie_stores_.end()) {
    return it->second.store.get();
  }

  EphemeralStore& entry = ephemeral_cookie_stores_[domain];
  entry.store = std::make_unique<CookieMonster_ChromiumImpl>(
      nullptr /* store */, net_log_.net_log());
  // AddCallbackForAllChanges registers under kGlobalDomainKey, which
  // DispatchChange only reaches when notify_global_hooks is true. Deletion
  // causes that pass false (DUPLICATE_IN_BACKING_STORE, DONT_RECORD, ALIAS,
  // LAST_ENTRY) therefore never reach this bridge. Those causes are not
  // web-observable; DUPLICATE_IN_BACKING_STORE cannot occur here (no persistent
  // store). EXPLICIT, OVERWRITE, EXPIRED and EVICTED do propagate.
  entry.forwarding_subscription =
      entry.store->GetChangeDispatcher().AddCallbackForAllChanges(
          base::BindRepeating(&CookieMonster::ForwardEphemeralChange,
                              base::Unretained(this)));
  return entry.store.get();
}

void CookieMonster::ForwardEphemeralChange(const CookieChangeInfo& change) {
  // Ephemeral stores are internal; surface their changes on the outer
  // dispatcher so RestrictedCookieManager bumps its shared-memory version and
  // renderers re-fetch. Global hooks stay off: extensions must not observe
  // ephemeral (3p) cookie writes.
  static_cast<CookieMonsterChangeDispatcher&>(
      CookieMonster_ChromiumImpl::GetChangeDispatcher())
      .DispatchChange(change, /*notify_global_hooks=*/false);
}

void CookieMonster::DeleteCanonicalCookieAsync(const CanonicalCookie& cookie,
                                               DeleteCallback callback) {
  for (auto& it : ephemeral_cookie_stores_) {
    it.second.store->DeleteCanonicalCookieAsync(cookie, DeleteCallback());
  }
  CookieMonster_ChromiumImpl::DeleteCanonicalCookieAsync(cookie,
                                                         std::move(callback));
}

void CookieMonster::DeleteAllCreatedInTimeRangeAsync(
    const CookieDeletionInfo::TimeRange& creation_range,
    DeleteCallback callback) {
  for (auto& it : ephemeral_cookie_stores_) {
    it.second.store->DeleteAllCreatedInTimeRangeAsync(creation_range,
                                                      DeleteCallback());
  }
  CookieMonster_ChromiumImpl::DeleteAllCreatedInTimeRangeAsync(
      creation_range, std::move(callback));
}

void CookieMonster::DeleteAllMatchingInfoAsync(CookieDeletionInfo delete_info,
                                               DeleteCallback callback) {
  if (delete_info.ephemeral_storage_domain.has_value()) {
    const std::string domain = *delete_info.ephemeral_storage_domain;
    auto it = ephemeral_cookie_stores_.find(domain);
    if (it == ephemeral_cookie_stores_.end()) {
      std::move(callback).Run(0);
      return;
    }

    // Take the entry out of the map first so a concurrent create for the same
    // domain gets a fresh store. Drain may complete synchronously; keep the
    // inner store alive until after that stack unwinds.
    EphemeralStore drained = std::move(it->second);
    ephemeral_cookie_stores_.erase(it);
    CookieMonster_ChromiumImpl* const store = drained.store.get();
    store->DeleteAllMatchingInfoAsync(
        CookieDeletionInfo(),
        base::BindOnce(
            [](EphemeralStore drained_store, DeleteCallback callback,
               uint32_t num_deleted) {
              std::move(callback).Run(num_deleted);
              base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
                  FROM_HERE, base::BindOnce([](EphemeralStore) {},
                                            std::move(drained_store)));
            },
            std::move(drained), std::move(callback)));
    return;
  }

  for (auto& it : ephemeral_cookie_stores_) {
    it.second.store->DeleteAllMatchingInfoAsync(delete_info, DeleteCallback());
  }
  CookieMonster_ChromiumImpl::DeleteAllMatchingInfoAsync(delete_info,
                                                         std::move(callback));
}

void CookieMonster::DeleteSessionCookiesAsync(DeleteCallback callback) {
  for (auto& it : ephemeral_cookie_stores_) {
    it.second.store->DeleteSessionCookiesAsync(DeleteCallback());
  }
  CookieMonster_ChromiumImpl::DeleteSessionCookiesAsync(std::move(callback));
}

void CookieMonster::SetCookieableSchemes(
    std::vector<std::string> schemes,
    SetCookieableSchemesCallback callback) {
  for (auto& it : ephemeral_cookie_stores_) {
    it.second.store->SetCookieableSchemes(schemes,
                                          SetCookieableSchemesCallback());
  }
  CookieMonster_ChromiumImpl::SetCookieableSchemes(std::move(schemes),
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
    CookieMonster_ChromiumImpl* ephemeral_monster =
        GetOrCreateEphemeralCookieStoreForTopFrameURL(
            options.top_frame_origin()->GetURL());
    ephemeral_monster->SetCanonicalCookieAsync(std::move(cookie), source_url,
                                               options, std::move(callback),
                                               std::move(cookie_access_result));
    return;
  }

  CookieMonster_ChromiumImpl::SetCanonicalCookieAsync(
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
    CookieMonster_ChromiumImpl* ephemeral_monster =
        GetOrCreateEphemeralCookieStoreForTopFrameURL(
            options.top_frame_origin()->GetURL());
    ephemeral_monster->GetCookieListWithOptionsAsync(
        url, options, cookie_partition_key_collection, std::move(callback));
    return;
  }

  CookieMonster_ChromiumImpl::GetCookieListWithOptionsAsync(
      url, options, cookie_partition_key_collection, std::move(callback));
}

}  // namespace net
