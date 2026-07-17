// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/signin/profile_customization_ui.h"

#include "chrome/grit/signin_resources.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/webui_util.h"

// profile_customization_ui.cc registers its JS files manually (it does not use
// a grit-generated resource map). Our chromium_src override of
// profile_customization_app.ts imports the upstream module as
// profile_customization_app-chromium.js, so that path must be served or the
// dialog fails to load and appears blank.
#define SetupWebUIDataSource(...)              \
  SetupWebUIDataSource(__VA_ARGS__);           \
  source->AddResourcePath(                     \
      "profile_customization_app-chromium.js", \
      IDR_SIGNIN_PROFILE_CUSTOMIZATION_PROFILE_CUSTOMIZATION_APP_CHROMIUM_JS)

#include <chrome/browser/ui/webui/signin/profile_customization_ui.cc>

#undef SetupWebUIDataSource
