#include "brave/vendor/brave-ios/components/bookmark_sync_service/bookmark_undo_service_factory.h"

#include "brave/vendor/brave-ios/components/keyed_service/browser_state_dependency_manager.h"
#include "brave/vendor/brave-ios/components/browser_state/chrome_browser_state.h"

#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "components/undo/bookmark_undo_service.h"

namespace brave {

// static
BookmarkUndoService* BookmarkUndoServiceFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<BookmarkUndoService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
BookmarkUndoService* BookmarkUndoServiceFactory::GetForBrowserStateIfExists(
    ChromeBrowserState* browser_state) {
  return static_cast<BookmarkUndoService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, false));
}

// static
BookmarkUndoServiceFactory* BookmarkUndoServiceFactory::GetInstance() {
  static base::NoDestructor<BookmarkUndoServiceFactory> instance;
  return instance.get();
}

BookmarkUndoServiceFactory::BookmarkUndoServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "BookmarkUndoService",
          BrowserStateDependencyManager::GetInstance()) {
}

BookmarkUndoServiceFactory::~BookmarkUndoServiceFactory() {
}

std::unique_ptr<KeyedService>
BookmarkUndoServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return base::WrapUnique(new BookmarkUndoService);
}

}  // namespace brave

