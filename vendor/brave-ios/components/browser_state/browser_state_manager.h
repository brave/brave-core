#ifndef browser_state_manager_h
#define browser_state_manager_h

#include <memory>
#include "base/macros.h"

class ChromeBrowserState;

class BrowserStateManager {
  public:
    static BrowserStateManager& instance();
    ~BrowserStateManager();
    
    ChromeBrowserState* getBrowserState();
    
  private:
    BrowserStateManager();
    DISALLOW_COPY_AND_ASSIGN(BrowserStateManager);
    
    std::unique_ptr<ChromeBrowserState> browser_state_;
};

#endif //browser_state_manager_h
