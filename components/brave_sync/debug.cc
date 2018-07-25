/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sstream>
#include "base/threading/platform_thread.h"
#include "content/public/browser/browser_thread.h"

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
