/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/common/user_agent.h"

#define BRAVE_GET_ANDROID_OS_INFO \
  include_android_model = IncludeAndroidModel::Exclude;

#include "../../../../content/common/user_agent.cc"

#undef BRAVE_GET_ANDROID_OS_INFO
