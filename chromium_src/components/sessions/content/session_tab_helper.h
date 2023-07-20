/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SESSIONS_CONTENT_SESSION_TAB_HELPER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SESSIONS_CONTENT_SESSION_TAB_HELPER_H_

// Include these here to avoid false positives with the redeclaration
#include "content/public/browser/web_contents_user_data.h"

#define WEB_CONTENTS_USER_DATA_KEY_DECL \
  friend class BraveSessionTabHelper;   \
  WEB_CONTENTS_USER_DATA_KEY_DECL

#include "src/components/sessions/content/session_tab_helper.h"  // IWYU pragma: export

#undef WEB_CONTENTS_USER_DATA_KEY_DECL

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SESSIONS_CONTENT_SESSION_TAB_HELPER_H_
