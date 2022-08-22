/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_GET_USER_AGENT_BRAND_LIST brand = version_info::GetProductName();

#include "src/components/embedder_support/user_agent_utils.cc"
#undef BRAVE_GET_USER_AGENT_BRAND_LIST
