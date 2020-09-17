#include "brave/vendor/brave-ios/components/bookmarks/startup_task_runner_service_factory.h"

#include "base/no_destructor.h"
#include "base/sequenced_task_runner.h"
#include "components/bookmarks/browser/startup_task_runner_service.h"
#include "brave/vendor/brave-ios/components/keyed_service/browser_state_dependency_manager.h"
#include "brave/vendor/brave-ios/components/browser_state/chrome_browser_state.h"

namespace brave {

// static
bookmarks::StartupTaskRunnerService*
StartupTaskRunnerServiceFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<bookmarks::StartupTaskRunnerService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
StartupTaskRunnerServiceFactory*
StartupTaskRunnerServiceFactory::GetInstance() {
  static base::NoDestructor<StartupTaskRunnerServiceFactory> instance;
  return instance.get();
}

StartupTaskRunnerServiceFactory::StartupTaskRunnerServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "StartupTaskRunnerService",
          BrowserStateDependencyManager::GetInstance()) {
}

StartupTaskRunnerServiceFactory::~StartupTaskRunnerServiceFactory() {
}

std::unique_ptr<KeyedService>
StartupTaskRunnerServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ChromeBrowserState* browser_state =
      ChromeBrowserState::FromBrowserState(context);
  return std::make_unique<bookmarks::StartupTaskRunnerService>(
      browser_state->GetIOTaskRunner());
}

}  // namespace brave
