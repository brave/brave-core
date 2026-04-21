/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcat/core/webcat_cache.h"

#include <utility>

#include "base/time/time.h"

namespace webcat {

WebcatCache::WebcatCache(size_t max_size, base::TimeDelta ttl)
    : max_size_(max_size), ttl_(ttl) {}

std::optional<CachedManifest> WebcatCache::Get(const url::Origin& origin) {
  auto it = entries_.find(origin);
  if (it == entries_.end()) {
    return std::nullopt;
  }

  auto& [manifest, lru_it] = it->second;

  if (base::Time::Now() > manifest.expiry_time) {
    lru_order_.erase(lru_it);
    entries_.erase(it);
    return std::nullopt;
  }

  Touch(lru_it);
  return manifest;
}

void WebcatCache::Put(const url::Origin& origin,
                      Bundle bundle,
                      const std::string& cid) {
  auto it = entries_.find(origin);
  if (it != entries_.end()) {
    lru_order_.erase(it->second.second);
    entries_.erase(it);
  }

  EvictIfNeeded();

  auto now = base::Time::Now();
  CachedManifest cached;
  cached.bundle = std::move(bundle);
  cached.cid = cid;
  cached.verification_time = now;
  cached.expiry_time = now + ttl_;

  lru_order_.push_front(origin);
  entries_.emplace(origin,
                   std::make_pair(std::move(cached), lru_order_.begin()));
}

void WebcatCache::Remove(const url::Origin& origin) {
  auto it = entries_.find(origin);
  if (it != entries_.end()) {
    lru_order_.erase(it->second.second);
    entries_.erase(it);
  }
}

void WebcatCache::Clear() {
  entries_.clear();
  lru_order_.clear();
}

bool WebcatCache::Contains(const url::Origin& origin) const {
  return entries_.contains(origin);
}

void WebcatCache::UpdateCid(const url::Origin& origin,
                             const std::string& new_cid) {
  auto it = entries_.find(origin);
  if (it != entries_.end()) {
    if (it->second.first.cid != new_cid) {
      lru_order_.erase(it->second.second);
      entries_.erase(it);
    }
  }
}

void WebcatCache::EvictIfNeeded() {
  while (entries_.size() >= max_size_ && !lru_order_.empty()) {
    auto oldest = lru_order_.back();
    lru_order_.pop_back();
    entries_.erase(oldest);
  }
}

void WebcatCache::Touch(typename std::list<url::Origin>::iterator it) {
  lru_order_.splice(lru_order_.begin(), lru_order_, it);
}

}  // namespace webcat
