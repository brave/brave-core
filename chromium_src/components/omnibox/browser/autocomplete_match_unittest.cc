// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/omnibox/browser/autocomplete_match.h"

#include "components/grit/brave_components_strings.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_starter_pack_data.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

TEST(BraveAutocompleteMatchTest, AskBraveSearchHasItsPlaceholder) {
  TemplateURLData data;
  data.starter_pack_id = static_cast<int>(
      template_url_starter_pack_data::StarterPackId::kAskBraveSearch);
  TemplateURL template_url(data);

  EXPECT_EQ(l10n_util::GetStringUTF16(
                IDS_OMNIBOX_ASK_BRAVE_SEARCH_SCOPE_PLACEHOLDER_TEXT),
            AutocompleteMatch::GetKeywordPlaceholder(&template_url));
}
