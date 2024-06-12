/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tabs/tabs_util.h"

#include "components/sessions/content/session_tab_helper.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/page_navigator.h"
#include "net/http/http_response_headers.h"

namespace brave_ads {

namespace {

constexpr int kHtmlClientErrorResponseCodeClass = 4;
constexpr int kHtmlServerErrorResponseCodeClass = 5;

}  // namespace

SessionID GetTabIdFromWebContents(content::WebContents* const web_contents) {
  return sessions::SessionTabHelper::IdForTab(web_contents);
}

bool HttpResponseHasErrorCode(
    const net::HttpResponseHeaders* const response_headers) {
  CHECK(response_headers);

  const int response_code_class = response_headers->response_code() / 100;
  return response_code_class == kHtmlClientErrorResponseCodeClass ||
         response_code_class == kHtmlServerErrorResponseCodeClass;
}

}  // namespace brave_ads
