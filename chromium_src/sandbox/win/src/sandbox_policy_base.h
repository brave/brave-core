/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SANDBOX_WIN_SRC_SANDBOX_POLICY_BASE_H_
#define BRAVE_CHROMIUM_SRC_SANDBOX_WIN_SRC_SANDBOX_POLICY_BASE_H_

#include "sandbox/win/src/sandbox_policy.h"

#define IsConfigured                                \
  IsConfigured() const override;                    \
  void SetShouldPatchModuleFileName(bool) override; \
  bool ShouldPatchModuleFileName

#define configured_                      \
  should_patch_module_filename_ = false; \
  bool configured_

#include "src/sandbox/win/src/sandbox_policy_base.h"  // IWYU pragma: export

#undef IsConfigured
#undef configured_

#endif  // BRAVE_CHROMIUM_SRC_SANDBOX_WIN_SRC_SANDBOX_POLICY_BASE_H_
