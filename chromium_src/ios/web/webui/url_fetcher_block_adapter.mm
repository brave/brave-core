/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web/webui/url_fetcher_block_adapter.h"

#include "services/network/public/cpp/simple_url_loader.h"

#define GetFinalURL                            \
  GetFinalURL();                               \
  }                                            \
                                               \
  response_ = url_loader_->TakeResponseInfo(); \
                                               \
  if (!response_body) {                        \
  void

#include "src/ios/web/webui/url_fetcher_block_adapter.mm"

#undef GetFinalURL
