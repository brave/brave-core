#ifdef startup_task_runner_service_factory_h
#define startup_task_runner_service_factory_h
#include <memory>

#include "base/macros.h"
#include "base/no_destructor.h"
#include "brave/vendor/brave-ios/components/keyed_service/browser_state_keyed_service_factory.h"

class ChromeBrowserState;

namespace bookmarks {
class StartupTaskRunnerService;
}

namespace brave {
// Singleton that owns all StartupTaskRunnerServices and associates them with
// ChromeBrowserState.
class StartupTaskRunnerServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static bookmarks::StartupTaskRunnerService* GetForBrowserState(
      ChromeBrowserState* browser_state);
  static StartupTaskRunnerServiceFactory* GetInstance();

 private:
  friend class base::NoDestructor<StartupTaskRunnerServiceFactory>;

  StartupTaskRunnerServiceFactory();
  ~StartupTaskRunnerServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(StartupTaskRunnerServiceFactory);
};

}  // namespace brave
#endif
