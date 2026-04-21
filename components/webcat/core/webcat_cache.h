/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBCAT_CORE_WEBCAT_CACHE_H_
#define BRAVE_COMPONENTS_WEBCAT_CORE_WEBCAT_CACHE_H_

#include <list>
#include <map>
#include <optional>
#include <string>

#include "base/time/time.h"
#include "brave/components/webcat/core/bundle_parser.h"
#include "brave/components/webcat/core/constants.h"
#include "url/origin.h"

namespace webcat {

struct CachedManifest {
  Bundle bundle;
  std::string cid;
  base::Time verification_time;
  base::Time expiry_time;
};

class WebcatCache {
 public:
  explicit WebcatCache(size_t max_size = kDefaultMaxCacheSize,
                       base::TimeDelta ttl = base::Seconds(kDefaultCacheTtlSeconds));

  std::optional<CachedManifest> Get(const url::Origin& origin);
  void Put(const url::Origin& origin, Bundle bundle, const std::string& cid);
  void Remove(const url::Origin& origin);
  void Clear();
  bool Contains(const url::Origin& origin) const;
  size_t size() const { return entries_.size(); }
  void UpdateCid(const url::Origin& origin, const std::string& new_cid);

 private:
  void EvictIfNeeded();
  void Touch(typename std::list<url::Origin>::iterator it);

  size_t max_size_;
  base::TimeDelta ttl_;
  std::list<url::Origin> lru_order_;
  std::map<url::Origin, std::pair<CachedManifest, std::list<url::Origin>::iterator>> entries_;
};

}  // namespace webcat

#endif  // BRAVE_COMPONENTS_WEBCAT_CORE_WEBCAT_CACHE_H_
