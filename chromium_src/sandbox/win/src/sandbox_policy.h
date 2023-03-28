/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SANDBOX_WIN_SRC_SANDBOX_POLICY_H_
#define BRAVE_CHROMIUM_SRC_SANDBOX_WIN_SRC_SANDBOX_POLICY_H_

#define IsConfigured                                   \
  IsConfigured() const = 0;                            \
  virtual void SetShouldPatchModuleFileName(bool) = 0; \
  virtual bool ShouldPatchModuleFileName

#include "src/sandbox/win/src/sandbox_policy.h"  // IWYU pragma: export

#undef IsConfigured

#endif  // BRAVE_CHROMIUM_SRC_SANDBOX_WIN_SRC_SANDBOX_POLICY_H_
