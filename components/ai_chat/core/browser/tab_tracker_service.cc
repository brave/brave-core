// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"

namespace ai_chat {

TabTrackerService::TabTrackerService() = default;
TabTrackerService::~TabTrackerService() = default;

void TabTrackerService::Bind(
    mojo::PendingReceiver<mojom::TabTrackerService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void TabTrackerService::UpdateTab(int32_t tab_id, mojom::TabDataPtr tab) {
  auto allowed_scheme =
      tab && kAllowedContentSchemes.contains(tab->url.scheme());

  auto it = std::ranges::find_if(
      tabs_, [tab_id](const auto& tab) { return tab->id == tab_id; });

  // New tab we haven't heard about
  if (it == tabs_.end()) {
    if (tab && allowed_scheme) {
      tabs_.push_back(std::move(tab));
    }
  } else if (!tab) {
    tabs_.erase(it);
  } else if (allowed_scheme) {
    // Update of an existing tab (on a valid scheme).
    *it = std::move(tab);
  } else {
    // Removal of an existing tab, or a tab navigated to a non-allowed scheme.
    tabs_.erase(it);
  }

  NotifyObservers();
}

void TabTrackerService::AddObserver(
    mojo::PendingRemote<mojom::TabDataObserver> observer) {
  auto id = observers_.Add(std::move(observer));

  NotifyObserver(observers_.Get(id));
}

void TabTrackerService::NotifyObservers() {
  if (observers_.empty()) {
    return;
  }

  for (auto& observer : observers_) {
    NotifyObserver(observer.get());
  }
}

void TabTrackerService::NotifyObserver(mojom::TabDataObserver* observer) {
  std::vector<mojom::TabDataPtr> state;
  std::ranges::transform(tabs_, std::back_inserter(state),
                         [](auto& tab) { return tab.Clone(); });
  observer->TabDataChanged(std::move(state));
}

}  // namespace ai_chat
