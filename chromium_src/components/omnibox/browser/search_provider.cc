/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/omnibox/browser/search_suggestion_parser.h"
#include "components/search_engines/search_engine_type.h"

// Pass additional argument to check if it's Brave Search's rich suggestion
#define ParseSuggestResults(root_list, input, scheme_classifier,            \
                            default_result_relevance, is_keyword_result,    \
                            results)                                        \
  ParseSuggestResults(root_list, input, scheme_classifier,                  \
                      default_result_relevance, is_keyword_result, results, \
                      IsBraveRichSuggestion(is_keyword_result))

#include "src/components/omnibox/browser/search_provider.cc"

#undef ParseSuggestResults

bool SearchProvider::IsBraveRichSuggestion(bool is_keyword) {
  auto* url = is_keyword ? providers_.GetKeywordProviderURL()
                         : providers_.GetDefaultProviderURL();
  return url && url->GetEngineType(
                    client()->GetTemplateURLService()->search_terms_data()) ==
                    SEARCH_ENGINE_BRAVE;
}
