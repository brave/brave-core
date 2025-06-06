/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/regional_capabilities/regional_capabilities_country_id.cc"

namespace regional_capabilities {

country_codes::CountryId CountryIdHolder::GetCountryCode() const {
  return country_id_;
}

}  // namespace regional_capabilities
