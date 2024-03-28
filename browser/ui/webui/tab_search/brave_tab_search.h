// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_TAB_SEARCH_BRAVE_TAB_SEARCH_H_
#define BRAVE_BROWSER_UI_WEBUI_TAB_SEARCH_BRAVE_TAB_SEARCH_H_

#include "base/task/cancelable_task_tracker.h"
#include "brave/browser/ui/webui/tab_search/brave_tab_search.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

class Profile;

class BraveTabSearch : public tab_search::mojom::BraveTabSearch {
 public:
  BraveTabSearch(
      Profile* profile,
      mojo::PendingReceiver<tab_search::mojom::BraveTabSearch> receiver);
  BraveTabSearch(const BraveTabSearch&) = delete;
  BraveTabSearch& operator=(const BraveTabSearch&) = delete;
  ~BraveTabSearch() override;

  // tab_search::mojom::BraveTabSearch
  void GetHistoryEntries(GetHistoryEntriesCallback callback) override;
  void OpenUrl(const GURL& url) override;

 private:
  raw_ptr<Profile> profile_;
  mojo::Receiver<tab_search::mojom::BraveTabSearch> reciever_;

  base::CancelableTaskTracker task_tracker_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_TAB_SEARCH_BRAVE_TAB_SEARCH_H_
