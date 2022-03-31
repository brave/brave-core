/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/common/user_agent.h"

namespace content {
std::string BuildModelInfo() {
  return std::string();
}
}  // namespace content

#define BRAVE_GET_ANDROID_OS_INFO \
  include_android_model = IncludeAndroidModel::Exclude;
#define BuildModelInfo BuildModelInfo_ChromiumImpl

#include "src/content/common/user_agent.cc"

#undef BuildModelInfo_ChromiumImpl
#undef BRAVE_GET_ANDROID_OS_INFO
