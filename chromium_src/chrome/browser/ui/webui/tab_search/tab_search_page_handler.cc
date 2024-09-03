#include "content/public/common/url_constants.h"
#include "url/gurl.h"

namespace {
GURL ReplaceChromeSchemeWithBrave(const GURL& url) {
  if (url.scheme() == content::kChromeUIScheme) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kBraveUIScheme);
    return url.ReplaceComponents(replacements);
  }
  return url;
}
}  // namespace

#include "src/chrome/browser/ui/webui/tab_search/tab_search_page_handler.cc"