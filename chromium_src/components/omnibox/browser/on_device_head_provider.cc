// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/search/search.h"
#include "components/search_engines/template_url_service.h"

namespace search {
bool IsOffTheRecordClient(bool is_off_the_record) {
  return is_off_the_record;
}
}  // namespace search

namespace {
bool BraveEnableOnHeadDeviceForAnyProvider(
    const TemplateURLService* template_url_service) {
  return true;
}
}  // namespace

#define DefaultSearchProviderIsGoogle(...)            \
  IsOffTheRecordClient(client()->IsOffTheRecord()) && \
      BraveEnableOnHeadDeviceForAnyProvider(__VA_ARGS__)

#include "src/components/omnibox/browser/on_device_head_provider.cc"

#undef DefaultSearchProviderIsGoogle
