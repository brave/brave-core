/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_PROCESSOR_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_PROCESSOR_UTIL_H_

#include <string>

namespace brave_ads {

std::string SanitizeHtml(const std::string& html);
std::string SanitizeText(const std::string& text);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_PROCESSOR_UTIL_H_
