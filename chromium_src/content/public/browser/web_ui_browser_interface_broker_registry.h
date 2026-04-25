// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_UI_BROWSER_INTERFACE_BROKER_REGISTRY_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_UI_BROWSER_INTERFACE_BROKER_REGISTRY_H_

// WebUIBrowserInterfaceBrokerRegistry::ForWebUI uses a DCHECK to enforce that
// all interfaces are registered with a particular WebUI at the same time. This
// is inconvenient for us because if we want to add our own interface
// registrations to an upstream UI we have to patch our registrations into the
// code where upstream registers theirs. To avoid this type of patching we are
// disabling this DCHECK restriction.
#define BRAVE_WEBUI_BROWSER_INTERFACE_BROKER_REGISTRY_FOR_WEBUI if (false)

#include <content/public/browser/web_ui_browser_interface_broker_registry.h>  // IWYU pragma: export

#undef BRAVE_WEBUI_BROWSER_INTERFACE_BROKER_REGISTRY_FOR_WEBUI

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_UI_BROWSER_INTERFACE_BROKER_REGISTRY_H_
