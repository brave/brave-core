/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/base/network_isolation_key.h"

#include "../../../../net/base/network_isolation_key.cc"

namespace net {

const base::Optional<url::Origin>&
NetworkIsolationKey::GetEffectiveTopFrameOrigin() const {
  DCHECK_EQ(original_top_frame_origin_.has_value(),
            top_frame_origin_.has_value());
  return top_frame_origin_;
}

}  // namespace net
