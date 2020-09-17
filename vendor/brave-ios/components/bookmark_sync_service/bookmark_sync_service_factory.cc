
#include "brave/vendor/brave-ios/components/bookmark_sync_service/bookmark_sync_service_factory.h"

#include "brave/vendor/brave-ios/components/keyed_service/browser_state_dependency_manager.h"
#include "brave/vendor/brave-ios/components/browser_state/chrome_browser_state.h"

#include "components/sync_bookmarks/bookmark_sync_service.h"
#include "brave/vendor/brave-ios/components/browser_state/browser_state_otr_helper.h"
#include "brave/vendor/brave-ios/components/bookmark_sync_service/bookmark_undo_service_factory.h"

namespace brave {

// static
sync_bookmarks::BookmarkSyncService*
BookmarkSyncServiceFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<sync_bookmarks::BookmarkSyncService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
BookmarkSyncServiceFactory* BookmarkSyncServiceFactory::GetInstance() {
  static base::NoDestructor<BookmarkSyncServiceFactory> instance;
  return instance.get();
}

BookmarkSyncServiceFactory::BookmarkSyncServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "BookmarkSyncServiceFactory",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(brave::BookmarkUndoServiceFactory::GetInstance());
}

BookmarkSyncServiceFactory::~BookmarkSyncServiceFactory() {}

std::unique_ptr<KeyedService>
BookmarkSyncServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ChromeBrowserState* browser_state =
      ChromeBrowserState::FromBrowserState(context);
  std::unique_ptr<sync_bookmarks::BookmarkSyncService> bookmark_sync_service(
      new sync_bookmarks::BookmarkSyncService(
          BookmarkUndoServiceFactory::GetForBrowserStateIfExists(
              browser_state)));
  return bookmark_sync_service;
}

web::BrowserState* BookmarkSyncServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave
