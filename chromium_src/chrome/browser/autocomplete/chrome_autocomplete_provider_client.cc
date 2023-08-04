// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/chrome/browser/autocomplete/chrome_autocomplete_provider_client.cc"
#include "brave/components/commander/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_COMMANDER)
#include "brave/browser/ui/commander/commander_service_factory.h"
#include "brave/components/commander/browser/commander_frontend_delegate.h"

commander::CommanderFrontendDelegate*
ChromeAutocompleteProviderClient::GetCommanderDelegate() {
  return commander::CommanderServiceFactory::GetForBrowserContext(profile_);
}
#endif
