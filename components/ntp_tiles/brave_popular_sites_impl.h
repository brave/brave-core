/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_TILES_BRAVE_POPULAR_SITES_IMPL_H_
#define BRAVE_COMPONENTS_NTP_TILES_BRAVE_POPULAR_SITES_IMPL_H_

#include <map>

#include "components/ntp_tiles/popular_sites_impl.h"

namespace ntp_tiles {

class BravePopularSitesImpl : public PopularSitesImpl {
 public:
  using PopularSitesImpl::PopularSitesImpl;

  BravePopularSitesImpl(const BravePopularSitesImpl&) = delete;
  BravePopularSitesImpl& operator=(const BravePopularSitesImpl&) = delete;

  // PopularSitesImpl overrides:
  const std::map<SectionType, SitesVector>& sections() const override;
};

}  // namespace ntp_tiles

#endif  // BRAVE_COMPONENTS_NTP_TILES_BRAVE_POPULAR_SITES_IMPL_H_
