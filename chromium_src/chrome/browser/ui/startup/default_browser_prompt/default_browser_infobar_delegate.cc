/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shell_integration.h"

#define DefaultBrowserWorker BraveDefaultBrowserWorker
#include "src/chrome/browser/ui/startup/default_browser_prompt/default_browser_infobar_delegate.cc"
#undef DefaultBrowserWorker
