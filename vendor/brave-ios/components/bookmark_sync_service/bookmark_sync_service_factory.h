
#ifndef bookmark_sync_service_factory_h
#define bookmark_sync_service_factory_h

#include "base/macros.h"
#include "base/no_destructor.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace sync_bookmarks {
class BookmarkSyncService;
}

class ChromeBrowserState;

namespace ios {

// Singleton that owns the bookmark sync service.
class BookmarkSyncServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  // Returns the instance of BookmarkSyncService associated with this profile
  // (creating one if none exists).
  static sync_bookmarks::BookmarkSyncService* GetForBrowserState(
      ChromeBrowserState* browser_state);

  // Returns an instance of the BookmarkSyncServiceFactory singleton.
  static BookmarkSyncServiceFactory* GetInstance();

 private:
  friend class base::NoDestructor<BookmarkSyncServiceFactory>;

  BookmarkSyncServiceFactory();
  ~BookmarkSyncServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(BookmarkSyncServiceFactory);
};

}  // namespace brave
#endif //bookmark_sync_service_factory_h
