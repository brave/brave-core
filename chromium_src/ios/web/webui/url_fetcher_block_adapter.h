/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_URL_FETCHER_BLOCK_ADAPTER_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_URL_FETCHER_BLOCK_ADAPTER_H_

#include "base/memory/raw_ptr.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

#define completion_handler_                                \
  completion_handler_;                                     \
                                                           \
 public:                                                   \
  const network::mojom::URLResponseHeadPtr getResponse() { \
    return response_.Clone();                              \
  }                                                        \
                                                           \
 private:                                                  \
  network::mojom::URLResponseHeadPtr response_

#include "src/ios/web/webui/url_fetcher_block_adapter.h"  // IWYU pragma: export

#undef completion_handler_

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_URL_FETCHER_BLOCK_ADAPTER_H_
