// Copyright (c) 2019 The Brave Authors
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/navigation_bar_data_provider.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/buildflags.h"
#include "content/public/browser/web_ui_data_source.h"

namespace {

void BraveAddBookmarksResources(content::WebUIDataSource* source,
                                Profile* profile) {
  NavigationBarDataProvider::Initialize(source, profile);
  source->AddLocalizedString("emptyList",
                             IDS_BRAVE_BOOKMARK_MANAGER_EMPTY_LIST);
}

}  // namespace

#define BRAVE_CREATE_BOOKMARKS_UI_HTML_SOURCE \
  BraveAddBookmarksResources(source, profile);

#include "src/chrome/browser/ui/webui/bookmarks/bookmarks_ui.cc"

#undef BRAVE_CREATE_BOOKMARKS_UI_HTML_SOURCE
