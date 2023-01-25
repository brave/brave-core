/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_MEDIA_HELPER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_MEDIA_HELPER_H_

#include <string>

namespace braveledger_media {

std::string GetMediaKey(const std::string& mediaId, const std::string& type);

std::string ExtractData(const std::string& data,
                        const std::string& match_after,
                        const std::string& match_until);

}  // namespace braveledger_media

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_MEDIA_HELPER_H_
