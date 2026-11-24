// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_SEMANTIC_HISTORY_SCOPE_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_SEMANTIC_HISTORY_SCOPE_H_

#include <string>

class AutocompleteProviderClient;
class TemplateURLService;

// Whether `keyword` selects the @history scope while the profile has semantic
// history search, the same check that gates its omnibox matches. Either
// pointer may be null, which means no.
bool IsSemanticHistoryScope(const std::u16string& keyword,
                            const TemplateURLService* template_url_service,
                            AutocompleteProviderClient* provider_client);

// The placeholder the @history scope shows while semantic history search is on.
std::u16string GetSemanticHistoryScopePlaceholder();

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_SEMANTIC_HISTORY_SCOPE_H_
