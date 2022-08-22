#include "brave/components/url_sanitizer/url_sanitizer.h"

#include "url/gurl.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

TEST(URLSanitizer, StripQueryParameter) {
  GURL initial_url("https://some.url/path?fbclid=11&param1=1");
  EXPECT_EQ(URLSanitizer::StripQueryParameter(initial_url.query(), initial_url.spec()), "param1=1");
}

}  // namespace brave
