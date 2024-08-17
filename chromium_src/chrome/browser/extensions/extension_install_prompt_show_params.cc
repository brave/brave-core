/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/extension_install_prompt_show_params.h"

#include "build/build_config.h"

#if BUILDFLAG(IS_MAC)
// On MacOS, the call to parent_web_contents_->GetTopLevelNativeWindow in
// ExtensionInstallPromptShowParams::ExtensionInstallPromptShowParams returns
// nullptr and hits CHECK_IS_TEST when this code is triggered from importing
// Chrome profile.
// Filed an upstream issue https://issues.chromium.org/issues/360321351
#include "base/check_is_test.h"
#undef CHECK_IS_TEST
#define CHECK_IS_TEST()
#endif

#include "src/chrome/browser/extensions/extension_install_prompt_show_params.cc"
#if BUILDFLAG(IS_MAC)
#undef CHECK_IS_TEST
#endif
