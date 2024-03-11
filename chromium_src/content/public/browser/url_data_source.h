/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_URL_DATA_SOURCE_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_URL_DATA_SOURCE_H_

#include <string>

#include "base/memory/ref_counted_memory.h"
#include "net/http/http_byte_range.h"

#define StartDataRequest                                                  \
  StartDataRequest_Unused() {}                                            \
                                                                          \
  struct CONTENT_EXPORT RangeDataResult {                                 \
    RangeDataResult();                                                    \
    RangeDataResult(RangeDataResult&&) noexcept;                          \
    RangeDataResult& operator=(RangeDataResult&&) noexcept;               \
    ~RangeDataResult();                                                   \
                                                                          \
    scoped_refptr<base::RefCountedMemory> buffer;                         \
    net::HttpByteRange range;                                             \
    int64_t file_size = 0;                                                \
    std::string mime_type;                                                \
  };                                                                      \
  using GotRangeDataCallback = base::OnceCallback<void(RangeDataResult)>; \
                                                                          \
  virtual void StartRangeDataRequest(                                     \
      const GURL& url, const WebContents::Getter& wc_getter,              \
      const net::HttpByteRange& range, GotRangeDataCallback callback) {}  \
                                                                          \
  virtual bool SupportsRangeRequests(const GURL& url) const;              \
                                                                          \
  virtual void StartDataRequest

#include "src/content/public/browser/url_data_source.h"  // IWYU pragma: export

#undef StartDataRequest

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_URL_DATA_SOURCE_H_
