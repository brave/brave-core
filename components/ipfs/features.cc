/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/features.h"

#include "base/feature_list.h"
#include "brave/components/ipfs/buildflags/buildflags.h"

namespace ipfs {
namespace features {

const base::Feature kIpfsFeature{
  "Ipfs",
#if BUILDFLAG(ENABLE_IPFS)
      base::FEATURE_ENABLED_BY_DEFAULT
#else
      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

}  // namespace features
}  // namespace ipfs
