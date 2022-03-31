// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/sidebar/sidebar_bookmarks_ui.h"

#include <string>
#include <utility>

#include "brave/browser/resources/sidebar/grit/sidebar_resources.h"
#include "brave/browser/resources/sidebar/grit/sidebar_resources_map.h"
#include "brave/browser/ui/webui/sidebar/sidebar_bookmarks_page_handler.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "chrome/grit/generated_resources.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/l10n/l10n_util.h"

SidebarBookmarksUI::SidebarBookmarksUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(kSidebarBookmarksHost);
  static constexpr webui::LocalizedString kLocalizedStrings[] = {
      {"bookmarksTitle", IDS_BOOKMARK_MANAGER_TITLE},
  };
  for (const auto& str : kLocalizedStrings) {
    std::u16string l10n_str = l10n_util::GetStringUTF16(str.id);
    source->AddString(str.name, l10n_str);
  }

  Profile* const profile = Profile::FromWebUI(web_ui);
  PrefService* prefs = profile->GetPrefs();
  source->AddBoolean(
      "bookmarksDragAndDropEnabled",
      prefs->GetBoolean(bookmarks::prefs::kEditBookmarksEnabled));

  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFavicon2));
  webui::SetupWebUIDataSource(
      source, base::make_span(kSidebarResources, kSidebarResourcesSize),
      IDR_SIDEBAR_BOOKMARKS_BOOKMARKS_HTML);
  content::WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                                source);
}

SidebarBookmarksUI::~SidebarBookmarksUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(SidebarBookmarksUI)

void SidebarBookmarksUI::BindInterface(
    mojo::PendingReceiver<sidebar::mojom::BookmarksPageHandlerFactory>
        receiver) {
  bookmarks_page_factory_receiver_.reset();
  bookmarks_page_factory_receiver_.Bind(std::move(receiver));
}

void SidebarBookmarksUI::CreateBookmarksPageHandler(
    mojo::PendingReceiver<sidebar::mojom::BookmarksPageHandler> receiver) {
  bookmarks_page_handler_ =
      std::make_unique<SidebarBookmarksPageHandler>(std::move(receiver));
}
