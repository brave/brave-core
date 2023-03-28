/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "sandbox/win/src/sandbox_policy_base.h"
#include "sandbox/win/src/policy_broker.h"

#define SetupBasicInterceptions(...) \
  SetupBasicInterceptions(__VA_ARGS__, config())

#include "src/sandbox/win/src/sandbox_policy_base.cc"

#undef SetupBasicInterceptions

namespace sandbox {

void ConfigBase::SetShouldPatchModuleFileName(bool b) {
  should_patch_module_filename_ = b;
}

bool ConfigBase::ShouldPatchModuleFileName() const {
  return should_patch_module_filename_;
}

}  // namespace sandbox
