#define AddProfilesExtraParts AddProfilesExtraParts_ChromiumImpl
#include "../../../../chrome/browser/profiles/chrome_browser_main_extra_parts_profiles.cc"
#undef AddProfilesExtraParts

#include "brave/browser/browser_context_keyed_service_factories.h"

namespace {

class BraveBrowserMainExtraPartsProfiles : public ChromeBrowserMainExtraPartsProfiles {
 public:
  BraveBrowserMainExtraPartsProfiles() : ChromeBrowserMainExtraPartsProfiles() {}

  void PreProfileInit() override {
    ChromeBrowserMainExtraPartsProfiles::PreProfileInit();
    brave::EnsureBrowserContextKeyedServiceFactoriesBuilt();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveBrowserMainExtraPartsProfiles);
};

}  // namespace

namespace chrome {

void AddProfilesExtraParts(ChromeBrowserMainParts* main_parts) {
  main_parts->AddParts(new BraveBrowserMainExtraPartsProfiles());
}

}  // namespace chrome
