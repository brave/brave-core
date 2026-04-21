/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBCAT_CORE_ORIGIN_STATE_H_
#define BRAVE_COMPONENTS_WEBCAT_CORE_ORIGIN_STATE_H_

#include <optional>
#include <string>

#include "brave/components/webcat/core/bundle_parser.h"
#include "brave/components/webcat/core/constants.h"
#include "url/origin.h"

namespace webcat {

class OriginStateData {
 public:
  OriginStateData(url::Origin origin, OriginState state);

  const url::Origin& origin() const { return origin_; }
  OriginState state() const { return state_; }
  WebcatError error() const { return error_; }
  const std::string& error_detail() const { return error_detail_; }
  const std::string& cid() const { return cid_; }
  const std::optional<Bundle>& bundle() const { return bundle_; }

  void SetCid(const std::string& cid);
  void SetBundle(Bundle bundle);
  void SetVerified();
  void SetFailed(WebcatError error, const std::string& detail);
  void ClearError();

 private:
  url::Origin origin_;
  OriginState state_ = OriginState::kUnverified;
  WebcatError error_ = WebcatError::kNone;
  std::string error_detail_;
  std::string cid_;
  std::optional<Bundle> bundle_;
};

}  // namespace webcat

#endif  // BRAVE_COMPONENTS_WEBCAT_CORE_ORIGIN_STATE_H_
