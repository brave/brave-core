/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/brave_tab_strip_model_delegate.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "chrome/browser/ui/browser_command_controller.h"

#define BrowserTabStripModelDelegate BraveTabStripModelDelegate

#include <chrome/browser/ui/browser.cc>

#undef BrowserTabStripModelDelegate
