// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_AUTOCOMPLETE_MODEL_AUTOCOMPLETE_PROVIDER_CLIENT_IMPL_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_AUTOCOMPLETE_MODEL_AUTOCOMPLETE_PROVIDER_CLIENT_IMPL_H_

#include "build/build_config.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"

#define GetAutocompleteClassifier                     \
  GetAutocompleteClassifier() override;               \
  void OpenLeo(const std::u16string& query) override; \
  bool IsLeoProviderEnabled
#include "src/ios/chrome/browser/autocomplete/model/autocomplete_provider_client_impl.h"  // IWYU pragma: export
#undef GetAutocompleteClassifier

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_AUTOCOMPLETE_MODEL_AUTOCOMPLETE_PROVIDER_CLIENT_IMPL_H_
