/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_omnibox_client.h"
#include "components/omnibox/browser/omnibox_client.h"
#include "components/omnibox/browser/omnibox_controller.h"
#include "url/gurl.h"

#if !defined(OS_IOS)
#include "content/public/common/url_constants.h"
#endif

class BraveOmniboxController : public OmniboxController {
 public:
  BraveOmniboxController(OmniboxEditModel* omnibox_edit_model,
                         OmniboxClient* client)
      : OmniboxController(omnibox_edit_model, client),
        client_(static_cast<BraveOmniboxClient*>(client)) {}
  BraveOmniboxController(const BraveOmniboxController&) = delete;
  BraveOmniboxController& operator=(const BraveOmniboxController&) = delete;
  ~BraveOmniboxController() override = default;

  // OmniboxController overrides:
  void StartAutocomplete(const AutocompleteInput& input) const override {
    if (!client_->IsAutocompleteEnabled())
      return;

    OmniboxController::StartAutocomplete(input);
  }

 private:
  BraveOmniboxClient* client_ = nullptr;
};

namespace {
void BraveAdjustTextForCopy(GURL* url) {
#if !defined(OS_IOS)
  if (url->scheme() == content::kChromeUIScheme) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kBraveUIScheme);
    *url = url->ReplaceComponents(replacements);
  }
#endif
}

}  // namespace

#define BRAVE_ADJUST_TEXT_FOR_COPY \
  BraveAdjustTextForCopy(url_from_text);

#define OmniboxController BraveOmniboxController
#include "src/components/omnibox/browser/omnibox_edit_model.cc"
#undef OmniboxController
#undef BRAVE_ADJUST_TEXT_FOR_COPY
