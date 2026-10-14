// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/text_param_util.h"

#include "base/values.h"

namespace gestures {

TextParamInfo ParseTextParam(const base::DictValue& dict) {
  TextParamInfo text_param;
  if (const std::string* value = dict.FindString(kTextParamKey)) {
    text_param.value = *value;
  }
  return text_param;
}

}  // namespace gestures
