/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shell_integration.h"

#define DefaultBrowserWorker BraveDefaultBrowserWorker
#include <chrome/browser/default_browser/setters/shell_integration_default_browser_setter.cc>
#undef DefaultBrowserWorker
