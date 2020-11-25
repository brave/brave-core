#include "brave/vendor/brave-ios/components/browser_state/bookmark_model_loaded_observer.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "ios/chrome/browser/sync/profile_sync_service_factory.h"

BookmarkModelLoadedObserver::BookmarkModelLoadedObserver(
    ChromeBrowserState* browser_state)
    : browser_state_(browser_state) {}

void BookmarkModelLoadedObserver::BookmarkModelChanged() {}

void BookmarkModelLoadedObserver::BookmarkModelLoaded(
    bookmarks::BookmarkModel* model,
    bool ids_reassigned) {
  // Causes lazy-load if sync is enabled.
  //TODO: FIX
  //ProfileSyncServiceFactory::GetForBrowserState(browser_state_);
  (void)browser_state_; //TODO: REMOVE
    fprintf(stderr, "MODEL: %p\n", model);
  model->RemoveObserver(this);
  delete this;
}

void BookmarkModelLoadedObserver::BookmarkModelBeingDeleted(
    bookmarks::BookmarkModel* model) {
  model->RemoveObserver(this);
  delete this;
}
