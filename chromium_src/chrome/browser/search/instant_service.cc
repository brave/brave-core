#include "chrome/browser/search/instant_service.h"
#include "chrome/browser/search/search_provider_observer.h"

class AlwaysGoogleSearchProviderObserver : public SearchProviderObserver {
 public:
  using SearchProviderObserver::SearchProviderObserver;
  // This is not override because base is not virtual, but that's ok if we
  // reference this derived class.
  bool is_google() { return true; }
};

#define SearchProviderObserver AlwaysGoogleSearchProviderObserver
#include "../../../../../chrome/browser/search/instant_service.cc"
#undef SearchProviderObserver
