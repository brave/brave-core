/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "sandbox/win/src/sandbox_policy.h"
#include "sandbox/win/src/sandbox_policy_base.h"

// `RendererFeatureSandboxWinTest` test now derives from
// `::sandbox::TargetConfig`. Since we previously added overrides to that class,
// we must add them here as well.
#define IsConfigured                         \
  IsConfigured() const override {            \
    return false;                            \
  }                                          \
  void SetShouldPatchModuleFileName(bool) {} \
  bool ShouldPatchModuleFileName

#include <content/browser/renderer_host/renderer_sandboxed_process_launcher_delegate_unittest.cc>  // IWYU pragma: export

#undef IsConfigured
