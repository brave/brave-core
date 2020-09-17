//
//  BraveBrowserStateManager.hpp
//  base/third_party/double_conversion:double_conversion
//
//  Created by brandon on 2020-05-12.
//

#ifndef BraveBrowserStateManager_h
#define BraveBrowserStateManager_h

#include <map>
#include <memory>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "ios/chrome/browser/browser_state/browser_state_info_cache.h"
#include "brave/vendor/brave-ios/components/browser_state/chrome_browser_state_manager.h"

namespace brave {
class BraveBrowserState;

class BraveBrowserStateManager : public ChromeBrowserStateManager {
 public:
  BraveBrowserStateManager();
  ~BraveBrowserStateManager() override;

  // ChromeBrowserStateManager:
  ChromeBrowserState* GetLastUsedBrowserState() override;
  ChromeBrowserState* GetBrowserState(const base::FilePath& path) override;
  BrowserStateInfoCache* GetBrowserStateInfoCache() override;
  std::vector<ChromeBrowserState*> GetLoadedBrowserStates() override;

 private:
  using ChromeBrowserStateImplPathMap =
      std::map<base::FilePath, std::unique_ptr<BraveBrowserState>>;
    
  base::FilePath GetLastUsedBrowserStateDir(
      const base::FilePath& user_data_dir);
    
  void DoFinalInit(ChromeBrowserState* browser_state);
  void DoFinalInitForServices(ChromeBrowserState* browser_state);
    
  void AddBrowserStateToCache(ChromeBrowserState* browser_state);
    
  ChromeBrowserStateImplPathMap browser_states_;
  std::unique_ptr<BrowserStateInfoCache> browser_state_info_cache_;

  DISALLOW_COPY_AND_ASSIGN(BraveBrowserStateManager);
};
}

#endif /* BraveBrowserStateManager_h */
