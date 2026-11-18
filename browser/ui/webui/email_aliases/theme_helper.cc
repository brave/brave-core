// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/email_aliases/theme_helper.h"

#include <utility>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "content/public/browser/web_ui_data_source.h"

namespace email_aliases {

void AddWebUIThemeSource(content::BrowserContext* context) {
  content::URLDataSource::Add(
      context,
      std::make_unique<ThemeSource>(Profile::FromBrowserContext(context)));
}

}  // namespace email_aliases
