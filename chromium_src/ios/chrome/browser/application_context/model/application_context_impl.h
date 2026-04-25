// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_APPLICATION_CONTEXT_MODEL_APPLICATION_CONTEXT_IMPL_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_APPLICATION_CONTEXT_MODEL_APPLICATION_CONTEXT_IMPL_H_

// Makes PreMainMessageLoopRun virtual so that we can override it in
// BraveApplicationContextImpl
#define PreMainMessageLoopRun virtual PreMainMessageLoopRun

#include <ios/chrome/browser/application_context/model/application_context_impl.h>  // IWYU pragma: export

#undef PreMainMessageLoopRun

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_APPLICATION_CONTEXT_MODEL_APPLICATION_CONTEXT_IMPL_H_
