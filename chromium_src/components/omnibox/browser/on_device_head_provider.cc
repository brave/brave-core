// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/omnibox/browser/on_device_head_provider.h"

#include "components/search/search.h"
#include "components/search_engines/template_url_service.h"

namespace search {
bool BraveEnableOnHeadDeviceForAnyProvider(
    const TemplateURLService* template_url_service) {
  return true;
}
}  // namespace search

#define SearchSuggestEnabled(...) SearchSuggestEnabled() && false

#define DefaultSearchProviderIsGoogle(...) \
  BraveEnableOnHeadDeviceForAnyProvider(__VA_ARGS__)

#include <components/omnibox/browser/on_device_head_provider.cc>

#undef DefaultSearchProviderIsGoogle
#undef SearchSuggestEnabled
