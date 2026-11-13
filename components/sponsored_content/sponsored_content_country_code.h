/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_COUNTRY_CODE_H_
#define BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_COUNTRY_CODE_H_

#include <string>

namespace variations {
class VariationsService;
}  // namespace variations

namespace sponsored_content {

// Returns the client's current country code, in ISO 3166-1 alpha-2 format.
// `variations_service` can be null in test.
std::string GetVariationsCountryCode(
    variations::VariationsService* variations_service);

}  // namespace sponsored_content

#endif  // BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_COUNTRY_CODE_H_
