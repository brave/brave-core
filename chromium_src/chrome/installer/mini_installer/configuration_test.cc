/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This file contains code that used to be upstream and had to be restored in
// Brave to support delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937

#include "chrome/installer/mini_installer/configuration.h"

// Need to include registry.h because it has an `Initialize` method, and we
// define a macro with the same name below.
#include "base/win/registry.h"

#define Initialize() Initialize(::GetModuleHandle(nullptr))

#include <chrome/installer/mini_installer/configuration_test.cc>

#undef Initialize
