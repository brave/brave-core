#include "chrome/browser/bookmarks/chrome_bookmark_client.h"
#include "brave/browser/bookmarks/brave_bookmark_client.h"
// Stop loading old Brave permanent nodes, GetEntityForBookmarkNode will fail
// see 9645795920481e2a80613a72aeddba1a65490dac
// #define ChromeBookmarkClient BraveBookmarkClient
#include "../../../../../../chrome/browser/bookmarks/bookmark_model_factory.cc"
