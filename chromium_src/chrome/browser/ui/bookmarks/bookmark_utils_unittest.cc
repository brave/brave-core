#include "chrome/browser/ui/bookmarks/bookmark_utils.h"

#include "content/public/common/url_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace chrome {
TEST(BraveBookmarkUtilsTest, BraveSchemeIsReplaced) {
  GURL url(content::kChromeUIScheme + std::string("://test"));
  std::u16string new_url = chrome::FormatBookmarkURLForDisplay(url);
  EXPECT_EQ(GURL(new_url).scheme(), content::kBraveUIScheme);
}
}  // namespace chrome
