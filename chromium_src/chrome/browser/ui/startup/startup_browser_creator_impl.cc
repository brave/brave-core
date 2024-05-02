/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/startup/brave_startup_tab_provider_impl.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/browser/ui/startup/startup_tab_provider.h"

#define StartupTabProviderImpl BraveStartupTabProviderImpl

#include "src/chrome/browser/ui/startup/startup_browser_creator_impl.cc"

#undef StartupTabProviderImpl
