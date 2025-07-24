// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/web_state/web_state_impl.h"

// Redirect WebUI messages to Brave's handler
#define HandleWebUIMessage HandleBraveWebUIMessage

#include <ios/web/webui/web_ui_messaging_java_script_feature.mm>

#undef HandleWebUIMessage
