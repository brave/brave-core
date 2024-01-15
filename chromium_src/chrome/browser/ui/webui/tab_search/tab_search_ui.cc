// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>

#include "brave/browser/ui/webui/tab_search/brave_tab_search.h"
#include "brave/browser/ui/webui/tab_search/brave_tab_search.mojom.h"
#include "chrome/browser/ui/ui_features.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

#define IsTabOrganization() IsTabOrganization()); \
  source->AddBoolean("tabSearchHistory", base::FeatureList::IsEnabled(features::kTabSearchHistory)

#include "src/chrome/browser/ui/webui/tab_search/tab_search_ui.cc"

#undef IsTabOrganization

void TabSearchUI::BindInterface(
    mojo::PendingReceiver<tab_search::mojom::BraveTabSearch> receiver) {
  auto* contents = web_ui()->GetWebContents();
  CHECK(contents);

  auto* profile = Profile::FromBrowserContext(contents->GetBrowserContext());
  CHECK(profile);

  brave_tab_search_ =
      std::make_unique<BraveTabSearch>(profile, std::move(receiver));
}
