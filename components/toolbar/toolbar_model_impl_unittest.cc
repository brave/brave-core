// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/toolbar/brave_toolbar_model_impl.h"

#include "base/strings/utf_string_conversions.h"
#include "components/toolbar/toolbar_model_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

class FakeToolbarModelDelegate : public ToolbarModelDelegate {
 public:
  void SetURL(const GURL& url) { url_ = url; }

  // ToolbarModelDelegate:
  base::string16 FormattedStringWithEquivalentMeaning(
      const GURL& url,
      const base::string16& formatted_url) const override {
    return formatted_url;
  }

  bool GetURL(GURL* url) const override {
    *url = url_;
    return true;
  }

 private:
  GURL url_;
};

TEST(BraveToolbarModelImplTest,
     DisplayUrlRewritesScheme) {
  FakeToolbarModelDelegate delegate;
  auto model = std::make_unique<BraveToolbarModelImpl>(&delegate, 1024);

  delegate.SetURL(GURL("chrome://page"));

  // Verify that both the full formatted URL and the display URL add the test
  // suffix.
  EXPECT_EQ(base::ASCIIToUTF16("brave://page"),
            model->GetURLForDisplay());
}

}  // namespace
