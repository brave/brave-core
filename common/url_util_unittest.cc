/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/url_util.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "url/gurl.h"

typedef testing::Test BraveUrlUtilTest;

namespace brave {

TEST_F(BraveUrlUtilTest, GetURLOrPDFURL) {
  std::vector<GURL> unchanged_urls({
    // PDFJS URL but not to a PDF
    GURL("chrome-extension://oemmndcbldboiebfnladdacbdfmadadm/test.html"),
    // PDFJS ID but not chrome-extension scheme
    GURL("chrome://oemmndcbldboiebfnladdacbdfmadadm/https://test.html"),
    // Not PDFJS ID but format of a PDFJS PDF URL
    GURL("chrome-extension://aaamndcbldboiebfnladdacbdfmadaaa/https://example.com/test.html"),
    // Random other URL
    GURL("https://example.com")
  });
  std::for_each(unchanged_urls.begin(), unchanged_urls.end(),
      [this](GURL url){
    EXPECT_EQ(brave::GetURLOrPDFURL(url), url);
  });
  EXPECT_EQ(brave::GetURLOrPDFURL(GURL("chrome-extension://oemmndcbldboiebfnladdacbdfmadadm/http://example.com?test")),
      GURL("http://example.com?test"));
  EXPECT_EQ(brave::GetURLOrPDFURL(GURL("chrome-extension://oemmndcbldboiebfnladdacbdfmadadm/https://example.com?test")),
      GURL("https://example.com?test"));
}

}  // namespace
