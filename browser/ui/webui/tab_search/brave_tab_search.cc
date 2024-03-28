// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/tab_search/brave_tab_search.h"

#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/webui/tab_search/brave_tab_search.mojom.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/keyed_service/core/service_access_type.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/common/referrer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

BraveTabSearch::BraveTabSearch(
    Profile* profile,
    mojo::PendingReceiver<tab_search::mojom::BraveTabSearch> reciever)
    : profile_(profile), reciever_(this, std::move(reciever)) {
  CHECK(profile);
}

BraveTabSearch::~BraveTabSearch() = default;

void BraveTabSearch::GetHistoryEntries(GetHistoryEntriesCallback callback) {
  auto* service = HistoryServiceFactory::GetForProfile(
      profile_, ServiceAccessType::EXPLICIT_ACCESS);
  if (!service || profile_->IsOffTheRecord()) {
    std::move(callback).Run({});
    return;
  }

  history::QueryOptions options;
  options.SetRecentDayRange(365);
  options.max_count = 1000;
  options.duplicate_policy = history::QueryOptions::REMOVE_ALL_DUPLICATES;
  options.visit_order = history::QueryOptions::RECENT_FIRST;
  service->QueryHistory(
      u"", options,
      base::BindOnce(
          [](GetHistoryEntriesCallback callback,
             history::QueryResults results) {
            std::vector<tab_search::mojom::HistoryEntryPtr> entries;
            for (const auto& result : results) {
              auto entry = tab_search::mojom::HistoryEntry::New();
              entry->title = base::UTF16ToUTF8(result.title());
              entry->url = result.url();
              entry->last_active_time = result.last_visit();
              entries.push_back(std::move(entry));
            }

            std::move(callback).Run(std::move(entries));
          },
          std::move(callback)),
      &task_tracker_);
}

void BraveTabSearch::OpenUrl(const GURL& url) {
  auto* browser = chrome::FindLastActiveWithProfile(profile_);
  CHECK(browser);

  content::OpenURLParams params(url, content::Referrer(),
                                WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                ui::PAGE_TRANSITION_FROM_ADDRESS_BAR, false);
  browser->OpenURL(params);
}
