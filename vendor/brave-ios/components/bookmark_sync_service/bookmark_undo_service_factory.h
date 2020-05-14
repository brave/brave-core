
#ifndef bookmark_undo_service_factory_h
#define bookmark_undo_service_factory_h

#include <memory>

#include "base/macros.h"
#include "base/no_destructor.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

class BookmarkUndoService;

class ChromeBrowserState;

namespace ios {
// Singleton that owns all FaviconServices and associates them with
// ChromeBrowserState.
class BookmarkUndoServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static BookmarkUndoService* GetForBrowserState(
      ChromeBrowserState* browser_state);
  static BookmarkUndoService* GetForBrowserStateIfExists(
      ChromeBrowserState* browser_state);
  static BookmarkUndoServiceFactory* GetInstance();

 private:
  friend class base::NoDestructor<BookmarkUndoServiceFactory>;

  BookmarkUndoServiceFactory();
  ~BookmarkUndoServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(BookmarkUndoServiceFactory);
};

}  // namespace ios
#endif //bookmark_undo_service_factory_h
