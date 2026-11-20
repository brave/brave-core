/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_CREATIVES_SEARCH_RESULT_AD_TEST_CREATIVE_SEARCH_RESULT_AD_TEST_UTIL_H_
#define BRAVE_BROWSER_BRAVE_ADS_CREATIVES_SEARCH_RESULT_AD_TEST_CREATIVE_SEARCH_RESULT_AD_TEST_UTIL_H_

namespace content {
class WebContents;
}  // namespace content

namespace brave_ads::test {

bool HasSearchResultAdClickedInfoBar(content::WebContents* web_contents);

}  // namespace brave_ads::test

#endif  // BRAVE_BROWSER_BRAVE_ADS_CREATIVES_SEARCH_RESULT_AD_TEST_CREATIVE_SEARCH_RESULT_AD_TEST_UTIL_H_
