/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sstream>
#include "base/threading/platform_thread.h"
#include "content/public/browser/browser_thread.h"
#include "components/bookmarks/browser/bookmark_node.h"

std::string GetThreadInfoString() {
   using content::BrowserThread;
   std::stringstream res;
   res << " tid="<< base::PlatformThread::CurrentId();
   res << " IsThreadInitialized(UI)=" << BrowserThread::IsThreadInitialized(BrowserThread::UI);
   res << " IsThreadInitialized(IO)=" << BrowserThread::IsThreadInitialized(BrowserThread::IO);
   BrowserThread::ID id = BrowserThread::ID_COUNT;
   bool bKnownThread =  BrowserThread::GetCurrentThreadIdentifier(&id);
   if (bKnownThread) {
     if (id == BrowserThread::UI) {
       res << " in UI THREAD";
     } else if (id == BrowserThread::IO){
       res << " in IO THREAD";
     } else {
       res << " in ??? THREAD";
     }
   } else {
     res << " UNKNOWN THREAD";
   }

   return res.str();
}

const char *GetBookmarkNodeString(bookmarks::BookmarkNode::Type type) {
  switch (type) {
    case bookmarks::BookmarkNode::URL:
      return "URL";
    case bookmarks::BookmarkNode::FOLDER:
      return "FOLDER";
    case bookmarks::BookmarkNode::BOOKMARK_BAR:
      return "BOOKMARK_BAR";
    case bookmarks::BookmarkNode::OTHER_NODE:
      return "OTHER_NODE";
    case bookmarks::BookmarkNode::MOBILE:
      return "MOBILE";
    default:
      NOTREACHED();
      return nullptr;
  }
}

bool ValidateBookmarksBaseOrder(const std::string &base_order) {
  DCHECK(base_order.size() >= 3);
  DCHECK(base_order.at(0) == '1' || base_order.at(0) == '2');
  DCHECK(base_order.at(1) == '.');
  return true;
}
