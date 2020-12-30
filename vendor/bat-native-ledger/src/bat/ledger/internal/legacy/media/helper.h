/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_HELPER_H_
#define BRAVELEDGER_MEDIA_HELPER_H_

#include <functional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"

namespace braveledger_media {

std::string GetMediaKey(const std::string& mediaId, const std::string& type);

void GetTwitchParts(
    const std::string& query,
    std::vector<base::flat_map<std::string, std::string>>* parts);

std::string ExtractData(const std::string& data,
                        const std::string& match_after,
                        const std::string& match_until);

void GetVimeoParts(
    const std::string& query,
    std::vector<base::flat_map<std::string, std::string>>* parts);

}  // namespace braveledger_media

#endif  // BRAVELEDGER_MEDIA_HELPER_H_
