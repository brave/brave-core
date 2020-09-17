//
//  chrome_browser_state.hpp
//  base/third_party/double_conversion:double_conversion
//
//  Created by brandon on 2020-05-07.
//

#ifndef chrome_browser_state_hpp
#define chrome_browser_state_hpp

#include "brave/vendor/brave-ios/components/browser_state/web_browser_state.h"

#include <map>
#include <string>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"

class PrefProxyConfigTracker;
class PrefService;

namespace base {
class SequencedTaskRunner;
class Time;
}

namespace sync_preferences {
class PrefServiceSyncable;
}

enum class ChromeBrowserStateType {
  REGULAR_BROWSER_STATE,
  INCOGNITO_BROWSER_STATE,
};

namespace brave {
class BraveBrowserStateIOData;

class ChromeBrowserState : public web::BrowserState {
 public:
  ~ChromeBrowserState() override;
    
  static ChromeBrowserState* FromBrowserState(BrowserState* browser_state);
    
  virtual scoped_refptr<base::SequencedTaskRunner> GetIOTaskRunner();
    
  virtual ChromeBrowserState* GetOriginalChromeBrowserState() = 0;
    
  virtual bool HasOffTheRecordChromeBrowserState() const = 0;
    
  virtual ChromeBrowserState* GetOffTheRecordChromeBrowserState() = 0;
    
  virtual void DestroyOffTheRecordChromeBrowserState() = 0;
    
  virtual PrefService* GetPrefs() = 0;
    
  virtual PrefService* GetOffTheRecordPrefs() = 0;
    
  virtual brave::BraveBrowserStateIOData* GetIOData() = 0;
    
  virtual sync_preferences::PrefServiceSyncable* GetSyncablePrefs();
    
  std::string GetDebugName();

 protected:
  explicit ChromeBrowserState(
      scoped_refptr<base::SequencedTaskRunner> io_task_runner);

 private:
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserState);
};
}

#endif /* chrome_browser_state_hpp */
