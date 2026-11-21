/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/location_bar/selected_keyword_view.h"

#include "brave/components/omnibox/browser/semantic_history_scope.h"
#include "chrome/browser/extensions/api/omnibox/omnibox_api.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/omnibox/omnibox_controller.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_starter_pack_data.h"
#include "ui/gfx/vector_icon_types.h"

// Defined in brave/browser/ui/views/location_bar.
const gfx::VectorIcon& GetAskBraveSearchStarterPackIcon();

namespace {

// Whether the keyword chip for `keyword` is the @history scope with semantic
// history search on.
bool IsSemanticHistoryKeyword(const std::u16string& keyword,
                              const OmniboxController* omnibox_controller,
                              Profile* profile) {
  const AutocompleteController* autocomplete_controller =
      omnibox_controller ? omnibox_controller->autocomplete_controller()
                         : nullptr;
  return IsSemanticHistoryScope(
      keyword, TemplateURLServiceFactory::GetForProfile(profile),
      autocomplete_controller
          ? autocomplete_controller->autocomplete_provider_client()
          : nullptr);
}

}  // namespace

// Sets the icon that is displayed when a Brave-defined starter pack engine is
// active.
#define kAiMode kAskBraveSearch) {                              \
    vector_icon = &GetAskBraveSearchStarterPackIcon();          \
    /* NOLINTNEXTLINE */                                        \
  } else if (template_url && template_url->starter_pack_id() == \
                                 template_url_starter_pack_data::StarterPackId::kAiMode

#include <chrome/browser/ui/views/location_bar/selected_keyword_view.cc>

#undef kAiMode
