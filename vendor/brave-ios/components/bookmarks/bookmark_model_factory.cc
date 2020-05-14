#include "brave/vendor/brave-ios/components/bookmarks/bookmark_model_factory.h"

#include <utility>

#include "base/no_destructor.h"
#include "base/deferred_sequenced_task_runner.h"
#include "base/task/post_task.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/browser/startup_task_runner_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "components/undo/bookmark_undo_service.h"
#include "brave/vendor/brave-ios/components/bookmarks/brave_bookmark_client.h"
#include "brave/vendor/brave-ios/components/bookmark_sync_service/bookmark_sync_service_factory.h"
#include "brave/vendor/brave-ios/components/bookmarks/startup_task_runner_service_factory.h"
#include "brave/vendor/brave-ios/components/bookmark_sync_service/bookmark_undo_service_factory.h"
#include "brave/vendor/brave-ios/components/browser_state/browser_state_otr_helper.h"
#include "brave/vendor/brave-ios/components/browser_state/chrome_browser_state.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"

using brave::BraveBookmarkClient;

namespace ios {

// static
bookmarks::BookmarkModel* BookmarkModelFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<bookmarks::BookmarkModel*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
bookmarks::BookmarkModel* BookmarkModelFactory::GetForBrowserStateIfExists(
    ChromeBrowserState* browser_state) {
  return static_cast<bookmarks::BookmarkModel*>(
      GetInstance()->GetServiceForBrowserState(browser_state, false));
}

// static
BookmarkModelFactory* BookmarkModelFactory::GetInstance() {
  static base::NoDestructor<BookmarkModelFactory> instance;
  return instance.get();
}

BookmarkModelFactory::BookmarkModelFactory()
    : BrowserStateKeyedServiceFactory(
          "BookmarkModel",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(BookmarkUndoServiceFactory::GetInstance());
  DependsOn(StartupTaskRunnerServiceFactory::GetInstance());
}

BookmarkModelFactory::~BookmarkModelFactory() {}

void BookmarkModelFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  bookmarks::RegisterProfilePrefs(registry);
}

std::unique_ptr<KeyedService> BookmarkModelFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ChromeBrowserState* browser_state =
      ChromeBrowserState::FromBrowserState(context);
  std::unique_ptr<bookmarks::BookmarkModel> bookmark_model(
      new bookmarks::BookmarkModel(std::make_unique<BraveBookmarkClient>(
          BookmarkSyncServiceFactory::GetForBrowserState(browser_state))));
  bookmark_model->Load(
      browser_state->GetPrefs(), browser_state->GetStatePath(),
      StartupTaskRunnerServiceFactory::GetForBrowserState(browser_state)
          ->GetBookmarkTaskRunner(),
      base::CreateSingleThreadTaskRunner({web::WebThread::UI}));
  BookmarkUndoServiceFactory::GetForBrowserState(browser_state)
      ->Start(bookmark_model.get());
  return bookmark_model;
}

web::BrowserState* BookmarkModelFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

bool BookmarkModelFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace ios
