// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tab_informer_service.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/mojom/tab_informer.mojom.h"

namespace ai_chat {

TabInformerService::TabInformerService() = default;
TabInformerService::~TabInformerService() = default;

void TabInformerService::Bind(
    mojo::PendingReceiver<mojom::TabInformer> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void TabInformerService::UpdateTab(int32_t tab_id, mojom::TabPtr tab) {
  auto allowed_scheme =
      tab && base::Contains(kAllowedContentSchemes, tab->url.scheme());

  auto it = base::ranges::find_if(
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

  NotifyListeners();
}

void TabInformerService::AddListener(
    mojo::PendingRemote<mojom::TabListener> listener) {
  auto id = listeners_.Add(std::move(listener));

  NotifyListener(listeners_.Get(id));
}

void TabInformerService::NotifyListeners() {
  if (listeners_.empty()) {
    return;
  }

  for (auto& listener : listeners_) {
    NotifyListener(listener.get());
  }
}

void TabInformerService::NotifyListener(mojom::TabListener* listener) {
  std::vector<mojom::TabPtr> state;
  std::ranges::transform(tabs_, std::back_inserter(state),
                         [](auto& tab) { return tab.Clone(); });
  listener->TabsChanged(std::move(state));
}

}  // namespace ai_chat
