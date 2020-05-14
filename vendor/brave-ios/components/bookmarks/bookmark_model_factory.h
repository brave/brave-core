#ifndef bookmark_model_factory_h
#define bookmark_model_factory_h

#include <memory>

#include "base/macros.h"
#include "base/no_destructor.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace bookmarks {
class BookmarkModel;
}

class ChromeBrowserState;

namespace ios {

class BookmarkModelFactory : public BrowserStateKeyedServiceFactory {
 public:
  static bookmarks::BookmarkModel* GetForBrowserState(
      ChromeBrowserState* browser_state);
  static bookmarks::BookmarkModel* GetForBrowserStateIfExists(
      ChromeBrowserState* browser_state);
  static BookmarkModelFactory* GetInstance();

 private:
  friend class base::NoDestructor<BookmarkModelFactory>;

  BookmarkModelFactory();
  ~BookmarkModelFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(BookmarkModelFactory);
};

}

#endif //bookmark_model_factory_h
