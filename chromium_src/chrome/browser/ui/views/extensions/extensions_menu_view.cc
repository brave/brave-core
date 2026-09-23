// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/extensions/extensions_menu_view.h"

#include "brave/browser/ui/views/extensions/brave_extension_menu_item_view.h"
#include "chrome/browser/ui/views/extensions/extensions_menu_item_view.h"

// Replace creation of ExtensionMenuItemView with our sub-classed
// implementation.
#define ExtensionMenuItemView(...) BraveExtensionMenuItemView(__VA_ARGS__)

#include <chrome/browser/ui/views/extensions/extensions_menu_view.cc>
#undef ExtensionMenuItemView
