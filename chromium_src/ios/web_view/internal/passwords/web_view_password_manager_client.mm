// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web_view/internal/passwords/web_view_password_manager_client.h"

// This override removes the account storage check since our version can use
// profile stored passwords
#define IsAccountStorageEnabled() IsAccountStorageEnabled() && false
#include <ios/web_view/internal/passwords/web_view_password_manager_client.mm>
#undef IsAccountStorageEnabled
