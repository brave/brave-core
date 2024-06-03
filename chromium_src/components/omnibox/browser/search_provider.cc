/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/omnibox/browser/search_suggestion_parser.h"

// Pass additional argument to check if it's Brave Search's rich suggestion
#define ParseSuggestResults(root_list, input, scheme_classifier,         \
                            default_result_relevance, is_keyword_result, \
                            results)                                     \
  ParseSuggestResults(                                                   \
      root_list, input, scheme_classifier, default_result_relevance,     \
      is_keyword_result, results,                                        \
      /*is_brave_rich_suggestion*/ providers_.GetDefaultProviderURL() && \
          providers_.GetDefaultProviderURL()->GetEngineType(             \
              client()->GetTemplateURLService()->search_terms_data()) == \
              SEARCH_ENGINE_BRAVE)

#include "src/components/omnibox/browser/search_provider.cc"

#undef ParseSuggestResults
