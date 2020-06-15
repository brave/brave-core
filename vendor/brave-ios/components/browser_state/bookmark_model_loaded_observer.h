#ifndef bookmark_model_loaded_observer_h
#define bookmark_model_loaded_observer_h

#include "base/macros.h"
#include "components/bookmarks/browser/base_bookmark_model_observer.h"

class ChromeBrowserState;

class BookmarkModelLoadedObserver : public bookmarks::BaseBookmarkModelObserver {
 public:
  explicit BookmarkModelLoadedObserver(ChromeBrowserState* browser_state);

 private:
  void BookmarkModelChanged() override;
  void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                           bool ids_reassigned) override;
  void BookmarkModelBeingDeleted(bookmarks::BookmarkModel* model) override;

  ChromeBrowserState* browser_state_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkModelLoadedObserver);
};

#endif //bookmark_model_loaded_observer_h
