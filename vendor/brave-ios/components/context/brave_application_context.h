//
//  brave_application_context.hpp
//  base/third_party/double_conversion:double_conversion
//
//  Created by brandon on 2020-05-12.
//

#ifndef brave_application_context_h
#define brave_application_context_h

#include "brave/vendor/brave-ios/components/context/application_context.h"

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"

namespace base {
class CommandLine;
class SequencedTaskRunner;
}

namespace brave {
class BraveApplicationContext : public ApplicationContext {
 public:
  BraveApplicationContext(base::SequencedTaskRunner* local_state_task_runner,
                         const base::CommandLine& command_line,
                         const std::string& locale);
  ~BraveApplicationContext() override;

  void StartTearDown();
  void PostDestroyThreads();

  // ApplicationContext implementation.
  void OnAppEnterForeground() override;
  void OnAppEnterBackground() override;
  bool WasLastShutdownClean() override;
  PrefService* GetLocalState() override;
    
  const std::string& GetApplicationLocale() override;
  brave::ChromeBrowserStateManager* GetChromeBrowserStateManager() override;
    
  IOSChromeIOThread* GetIOSChromeIOThread() override;

 private:
  void SetApplicationLocale(const std::string& locale);
  void CreateLocalState();

  base::ThreadChecker thread_checker_;

  std::unique_ptr<PrefService> local_state_;
  std::unique_ptr<IOSChromeIOThread> ios_chrome_io_thread_;
  std::unique_ptr<ChromeBrowserStateManager> chrome_browser_state_manager_;
  std::string application_locale_;
    
  const scoped_refptr<base::SequencedTaskRunner> local_state_task_runner_;
    

  bool was_last_shutdown_clean_;

  DISALLOW_COPY_AND_ASSIGN(BraveApplicationContext);
};
}

#endif /* brave_application_context_h */
