/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_OMNIBOX_BRAVE_OMNIBOX_CLIENT_IMPL_H_
#define BRAVE_BROWSER_UI_OMNIBOX_BRAVE_OMNIBOX_CLIENT_IMPL_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"
#include "chrome/browser/ui/omnibox/chrome_omnibox_client.h"

class OmniboxEditController;
class PrefRegistrySimple;
class Profile;

class BraveOmniboxClientImpl : public ChromeOmniboxClient {
 public:
  BraveOmniboxClientImpl(OmniboxEditController* controller, Profile* profile);
  BraveOmniboxClientImpl(const BraveOmniboxClientImpl&) = delete;
  BraveOmniboxClientImpl& operator=(const BraveOmniboxClientImpl&) = delete;
  ~BraveOmniboxClientImpl() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* prefs);

  const AutocompleteSchemeClassifier& GetSchemeClassifier() const override;
  bool IsAutocompleteEnabled() const override;

  void OnInputAccepted(const AutocompleteMatch& match) override;
  void OnTextChanged(const AutocompleteMatch& current_match,
                     bool user_input_in_progress,
                     const std::u16string& user_text,
                     const AutocompleteResult& result,
                     bool has_focus) override;
  void OnURLOpenedFromOmnibox(OmniboxLog* log) override;

 private:
  std::u16string user_text_;
  raw_ptr<Profile> profile_ = nullptr;
  BraveAutocompleteSchemeClassifier scheme_classifier_;
};

#endif  // BRAVE_BROWSER_UI_OMNIBOX_BRAVE_OMNIBOX_CLIENT_IMPL_H_
