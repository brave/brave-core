// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_AUTOCOMPLETE_CHROME_AUTOCOMPLETE_PROVIDER_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_AUTOCOMPLETE_CHROME_AUTOCOMPLETE_PROVIDER_CLIENT_H_

#include "brave/components/commander/common/buildflags/buildflags.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"

#if BUILDFLAG(ENABLE_COMMANDER)
#include "brave/components/commander/browser/commander_frontend_delegate.h"
#define GetAutocompleteClassifier       \
  GetAutocompleteClassifier() override; \
  commander::CommanderFrontendDelegate* GetCommanderDelegate
#endif  // BUILDFLAG(ENABLE_COMMANDER)

#if BUILDFLAG(ENABLE_AI_CHAT)
#define GetInMemoryDatabase                           \
  GetInMemoryDatabase() override;                     \
  void OpenLeo(const std::u16string& query) override; \
  bool IsLeoProviderEnabled
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#define GetAcceptLanguages             \
  GetAcceptLanguages() const override; \
  std::u16string GetClipboardText

#include "src/chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"  // IWYU pragma: export

#undef GetAcceptLanguages
#undef GetInMemoryDatabase
#undef GetAutocompleteClassifier

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_AUTOCOMPLETE_CHROME_AUTOCOMPLETE_PROVIDER_CLIENT_H_
