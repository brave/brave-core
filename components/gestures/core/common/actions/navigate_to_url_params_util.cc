// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/navigate_to_url_params_util.h"

#include "base/values.h"

namespace gestures {

NavigateToUrlParamsInfo ParseNavigateToUrlParams(
    const base::DictValue& action_dict) {
  NavigateToUrlParamsInfo navigate_to_url_params;
  if (const std::string* url = action_dict.FindString("navigate_to_url")) {
    navigate_to_url_params.url_spec = *url;
  }
  return navigate_to_url_params;
}

}  // namespace gestures
