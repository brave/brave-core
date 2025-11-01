/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_CREATE_PREFERENCES_FROM_POLICIES \
  pref_interceptor_.InterceptPrefValues(prefs.get());

#include <components/policy/core/browser/configuration_policy_pref_store.cc>

#undef BRAVE_CREATE_PREFERENCES_FROM_POLICIES
