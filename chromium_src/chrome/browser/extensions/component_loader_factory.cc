/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/component_loader_factory.h"

#include "base/memory/ptr_util.h"
#include "brave/browser/extensions/brave_component_loader.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/extensions_browser_client.h"

#define WrapUnique(...) WrapUnique(new BraveComponentLoader(profile))
#include "src/chrome/browser/extensions/component_loader_factory.cc"
#undef WrapUnique
