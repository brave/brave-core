// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_THEME_HELPER_H_
#define BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_THEME_HELPER_H_

namespace content {
class BrowserContext;
}

namespace email_aliases {
void AddWebUIThemeSource(content::BrowserContext* context);
}

#endif  // BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_THEME_HELPER_H_
