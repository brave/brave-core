/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_INFOBARS_CORE_CONFIRM_INFOBAR_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_INFOBARS_CORE_CONFIRM_INFOBAR_DELEGATE_H_

#include "build/build_config.h"

// Upstream deleted extra button but we use it foor infobar on desktop.
#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
#define BUTTON_CANCEL BUTTON_EXTRA = 1 << 2, BUTTON_CANCEL
#endif

#include "src/components/infobars/core/confirm_infobar_delegate.h"  // IWYU pragma: export

#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
#undef BUTTON_CANCEL
#endif

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_INFOBARS_CORE_CONFIRM_INFOBAR_DELEGATE_H_
