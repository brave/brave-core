//
//  chrome_state_manager.hpp
//  Sources
//
//  Created by brandon on 2020-05-12.
//

#ifndef browser_state_manager_h
#define browser_state_manager_h

#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"

namespace base {
class FilePath;
}

class BrowserStateInfoCache;

namespace brave {
class ChromeBrowserState;


class ChromeBrowserStateManager {
 public:
  virtual ~ChromeBrowserStateManager() {}
    
  virtual ChromeBrowserState* GetLastUsedBrowserState() = 0;
  virtual ChromeBrowserState* GetBrowserState(const base::FilePath& path) = 0;
  virtual BrowserStateInfoCache* GetBrowserStateInfoCache() = 0;
  virtual std::vector<ChromeBrowserState*> GetLoadedBrowserStates() = 0;

 protected:
  ChromeBrowserStateManager() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserStateManager);
};

}

#endif /* chrome_state_manager_h */
