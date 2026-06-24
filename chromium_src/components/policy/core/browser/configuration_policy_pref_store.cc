/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/policy/core/browser/configuration_policy_handler_list.h"

#define ApplyPolicySettings(...)    \
  ApplyPolicySettings(__VA_ARGS__); \
  pref_interceptor_.InterceptPrefValues(prefs.get())

#include <components/policy/core/browser/configuration_policy_pref_store.cc>

#undef ApplyPolicySettings
