/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcat/core/origin_state.h"

namespace webcat {

OriginStateData::OriginStateData(url::Origin origin, OriginState state)
    : origin_(std::move(origin)), state_(state) {}

void OriginStateData::SetCid(const std::string& cid) {
  cid_ = cid;
}

void OriginStateData::SetBundle(Bundle bundle) {
  bundle_ = std::move(bundle);
  state_ = OriginState::kBundleFetched;
  error_ = WebcatError::kNone;
  error_detail_.clear();
}

void OriginStateData::SetVerified() {
  state_ = OriginState::kVerified;
  error_ = WebcatError::kNone;
  error_detail_.clear();
}

void OriginStateData::SetFailed(WebcatError error, const std::string& detail) {
  state_ = OriginState::kFailed;
  error_ = error;
  error_detail_ = detail;
}

void OriginStateData::ClearError() {
  error_ = WebcatError::kNone;
  error_detail_.clear();
}

}  // namespace webcat
