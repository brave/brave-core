// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/omnibox/browser/semantic_history_scope.h"

#include "components/grit/brave_components_strings.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_starter_pack_data.h"
#include "ui/base/l10n/l10n_util.h"

bool IsSemanticHistoryScope(const std::u16string& keyword,
                            const TemplateURLService* template_url_service,
                            AutocompleteProviderClient* provider_client) {
  if (keyword.empty() || !template_url_service || !provider_client) {
    return false;
  }
  const TemplateURL* template_url =
      template_url_service->GetTemplateURLForKeyword(keyword);
  return template_url &&
         template_url->starter_pack_id() ==
             template_url_starter_pack_data::StarterPackId::kHistory &&
         provider_client->GetBraveHistoryEmbeddingsSearch();
}

std::u16string GetSemanticHistoryScopePlaceholder() {
  return l10n_util::GetStringUTF16(
      IDS_OMNIBOX_SEMANTIC_HISTORY_SCOPE_PLACEHOLDER_TEXT);
}
