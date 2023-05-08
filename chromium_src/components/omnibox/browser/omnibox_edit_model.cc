/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/omnibox/browser/omnibox_edit_model.h"

#include "base/memory/raw_ptr.h"
#include "brave/components/omnibox/browser/brave_omnibox_client.h"
#include "components/omnibox/browser/omnibox_client.h"
#include "components/omnibox/browser/omnibox_controller.h"
#include "url/gurl.h"

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
#include "brave/components/commander/common/constants.h"
#include "brave/components/commander/common/features.h"
#endif

#if !BUILDFLAG(IS_IOS)
#include "content/public/common/url_constants.h"
#endif

class BraveOmniboxController : public OmniboxController {
 public:
  BraveOmniboxController(OmniboxView* view,
                         OmniboxEditModelDelegate* edit_model_delegate,
                         std::unique_ptr<OmniboxClient> client)
      : OmniboxController(view, edit_model_delegate, std::move(client)) {}
  BraveOmniboxController(
      OmniboxEditModelDelegate* edit_model_delegate,
      std::unique_ptr<AutocompleteController> autocomplete_controller,
      std::unique_ptr<OmniboxClient> client)
      : OmniboxController(edit_model_delegate,
                          std::move(autocomplete_controller),
                          std::move(client)) {}
  BraveOmniboxController(const BraveOmniboxController&) = delete;
  BraveOmniboxController& operator=(const BraveOmniboxController&) = delete;
  ~BraveOmniboxController() override = default;

  // OmniboxController overrides:
  void StartAutocomplete(const AutocompleteInput& input) const override {
    auto* client = static_cast<BraveOmniboxClient*>(client_.get());
    if (!client->IsAutocompleteEnabled()) {
      return;
    }

    OmniboxController::StartAutocomplete(input);
  }
};

namespace {
void BraveAdjustTextForCopy(GURL* url) {
#if !BUILDFLAG(IS_IOS)
  if (url->scheme() == content::kChromeUIScheme) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kBraveUIScheme);
    *url = url->ReplaceComponents(replacements);
  }
#endif
}

}  // namespace

#define BRAVE_ADJUST_TEXT_FOR_COPY BraveAdjustTextForCopy(url_from_text);

#define CanPasteAndGo CanPasteAndGo_Chromium
#define OmniboxController BraveOmniboxController
#include "src/components/omnibox/browser/omnibox_edit_model.cc"
#undef OmniboxController
#undef CanPasteAndGo
#undef BRAVE_ADJUST_TEXT_FOR_COPY

bool OmniboxEditModel::CanPasteAndGo(const std::u16string& text) const {
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  if (commander::CommanderEnabled() &&
      base::StartsWith(text, commander::kCommandPrefix)) {
    return false;
  }
#endif
  return CanPasteAndGo_Chromium(text);
}
