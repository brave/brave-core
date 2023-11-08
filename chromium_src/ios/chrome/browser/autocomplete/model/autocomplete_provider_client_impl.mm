// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/ios/chrome/browser/autocomplete/model/autocomplete_provider_client_impl.mm"

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "build/build_config.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
void AutocompleteProviderClientImpl::OpenLeo(const std::u16string& query) {}

bool AutocompleteProviderClientImpl::IsLeoProviderEnabled() {
  return false;
}
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
