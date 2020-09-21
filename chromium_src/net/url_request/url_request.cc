/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_REDIRECT_CLEARREFERRER                                     \
if (removed_headers) {                                                  \
  if (removed_headers->end() != std::find(removed_headers->begin(),     \
                                          removed_headers->end(),       \
                                          "X-Brave-Clear-Referer")) {   \
    referrer_.clear();                                                  \
  }                                                                     \
}

#include "../../../../../net/url_request/url_request.cc"

#undef BRAVE_REDIRECT_CLEARREFERRER
