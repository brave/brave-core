// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_PASSWORDS_WEB_VIEW_PASSWORD_MANAGER_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_PASSWORDS_WEB_VIEW_PASSWORD_MANAGER_CLIENT_H_

class BraveWebViewPasswordManagerClient;

// Add a friend class so that subclasses can access the private `bridge_` field
#define requirements_service_ \
  requirements_service_;      \
  friend class ::BraveWebViewPasswordManagerClient
#include <ios/web_view/internal/passwords/web_view_password_manager_client.h>  // IWYU pragma: export
#undef requirements_service_

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_PASSWORDS_WEB_VIEW_PASSWORD_MANAGER_CLIENT_H_
