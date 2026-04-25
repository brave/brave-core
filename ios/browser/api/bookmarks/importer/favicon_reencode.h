/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_FAVICON_REENCODE_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_FAVICON_REENCODE_H_

#include <stddef.h>

#include <vector>


namespace importer {

// Given raw image data, decodes the icon, re-sampling to the correct size as
// necessary, and re-encodes as PNG data in the given output vector. Returns
// true on success.
bool ReencodeFavicon(const unsigned char* src_data,
                     size_t src_len,
                     std::vector<unsigned char>* png_data);

}  // namespace importer

#endif  // BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_FAVICON_REENCODE_H_
