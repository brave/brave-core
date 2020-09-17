//
//  application_context.hpp
//  Sources
//
//  Created by brandon on 2020-05-12.
//

#ifndef application_context_h
#define application_context_h

#include <string>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"

namespace brave {
class ChromeBrowserStateManager;
}

class IOSChromeIOThread;
class PrefService;

namespace brave {
class ApplicationContext;
    
// Gets the global application context. Cannot return null.
ApplicationContext* GetApplicationContext();

class ApplicationContext {
 public:
  ApplicationContext();
  virtual ~ApplicationContext();
    
  virtual void OnAppEnterForeground() = 0;
  virtual void OnAppEnterBackground() = 0;

  // Returns whether the last complete shutdown was clean (i.e. happened while
  // the application was backgrounded).
  virtual bool WasLastShutdownClean() = 0;

  // Gets the local state associated with this application.
  virtual PrefService* GetLocalState() = 0;

  // Gets the locale used by the application.
  virtual const std::string& GetApplicationLocale() = 0;

  // Gets the ChromeBrowserStateManager used by this application.
  virtual brave::ChromeBrowserStateManager* GetChromeBrowserStateManager() = 0;
  
  // Gets the IOSChromeIOThread.
  virtual IOSChromeIOThread* GetIOSChromeIOThread() = 0;

 protected:
  // Sets the global ApplicationContext instance.
  static void SetApplicationContext(ApplicationContext* context);

 private:
  DISALLOW_COPY_AND_ASSIGN(ApplicationContext);
};
}

#endif /* application_context_h */
