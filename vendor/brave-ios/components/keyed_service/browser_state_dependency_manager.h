//
//  browser_state_dependency_manager.h
//
//  Created by brandon on 2020-05-08.
//

#ifndef browser_state_dependency_manager_h
#define browser_state_dependency_manager_h

#include "base/callback_forward.h"
#include "base/callback_list.h"
#include "base/macros.h"
#include "components/keyed_service/core/dependency_manager.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace web {
class BrowserState;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave {
class BrowserStateDependencyManager : public DependencyManager {
 public:
  static BrowserStateDependencyManager* GetInstance();

  void RegisterBrowserStatePrefsForServices(
      user_prefs::PrefRegistrySyncable* registry);
        
  void CreateBrowserStateServices(web::BrowserState* context);
        
  void CreateBrowserStateServicesForTest(web::BrowserState* context);
        
  void DestroyBrowserStateServices(web::BrowserState* context);
        
  void AssertBrowserStateWasntDestroyed(web::BrowserState* context) const;
        
  void MarkBrowserStateLive(web::BrowserState* context);

 private:
  friend struct base::DefaultSingletonTraits<BrowserStateDependencyManager>;

  BrowserStateDependencyManager();
  ~BrowserStateDependencyManager() override;
        
  void DoCreateBrowserStateServices(web::BrowserState* context,
                                    bool is_testing_context);

#ifndef NDEBUG
  void DumpContextDependencies(void* context) const final;
#endif  // NDEBUG

  DISALLOW_COPY_AND_ASSIGN(BrowserStateDependencyManager);
};
}

#endif /* browser_state_dependency_manager_h */
