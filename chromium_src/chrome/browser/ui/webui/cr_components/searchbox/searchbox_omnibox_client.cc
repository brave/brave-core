// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/cr_components/searchbox/searchbox_omnibox_client.h"

#include <string_view>

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_search/common/features.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/page_navigator.h"
#include "net/base/url_util.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/brave_stats/first_run_util.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "chrome/browser/browser_process.h"
#endif

namespace {

// When `match` is a keyword search against Brave Search, tag the destination
// URL with `source=newtab` so Brave Search can distinguish NTP searchbox
// traffic. Plumbing the source through upstream's `{source}` replacement isn't
// viable: it only works for Google Search, and it can't tell the NTP realbox
// and the omnibox apart. The accompanying plaster wraps the `OpenURL()` params
// in `OnAutocompleteAccept()` with this helper.
content::OpenURLParams MaybeOverrideURLParams(
    content::OpenURLParams params,
    const AutocompleteMatch& match,
    TemplateURLService* template_url_service) {
  if (match.keyword.empty()) {
    return params;
  }

  const TemplateURL* template_url =
      template_url_service->GetTemplateURLForKeyword(match.keyword);
  if (template_url &&
      template_url->prepopulate_id() ==
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE) {
    std::string_view source = "newtab";
    if (brave_search::features::IsSearchNewTabV1SourceEnabled()) {
      source = "newtab_v1";
    }
#if BUILDFLAG(ENABLE_AI_CHAT)
    auto* local_state = g_browser_process->local_state();
    if (ai_chat::features::IsShowAIChatInputOnNewTabPageEnabled(
            local_state, brave_stats::IsFirstRun(local_state))) {
      source = "newtab_v2";
    }
#endif
    params.url =
        net::AppendOrReplaceQueryParameter(params.url, "source", source);
  }

  return params;
}

}  // namespace

#include <chrome/browser/ui/webui/cr_components/searchbox/searchbox_omnibox_client.cc>
