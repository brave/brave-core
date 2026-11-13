/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_H_

#include <memory>
#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/sponsored_content/sponsored_content_service_observer.h"
#include "components/prefs/pref_change_registrar.h"

namespace base {
class DictValue;
}  // namespace base

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace sponsored_content {
class SponsoredContentService;
}  // namespace sponsored_content

class PrefRegistrySimple;
class PrefService;

namespace ntp_background_images {

struct NTPBackgroundImagesData;
struct NTPSponsoredContentData;
struct NTPSponsoredSitesData;

class NTPBackgroundImagesService
    : public sponsored_content::SponsoredContentServiceObserver {
 public:
  class Observer {
   public:
    // Called when the background images component is updated.
    virtual void OnBackgroundImagesDataDidUpdate(
        NTPBackgroundImagesData* data) {}

    // Called when the sponsored content component is updated. This is
    // deprecated, use `OnSponsoredContentDidUpdate`.
    virtual void DeprecatedOnSponsoredContentDidUpdate(
        NTPSponsoredContentData* data) {}

    // Called when the sponsored content component is updated.
    virtual void OnSponsoredContentDidUpdate(const base::DictValue& data) {}

    // Called when the sponsored sites component is updated.
    virtual void OnSponsoredSitesDataDidUpdate() {}

    // Independent of `OnSponsoredContentDidUpdate`, since conversion
    // extraction applies to every ad format, not just New Tab Page ads.
    virtual void OnSponsoredConversionConfigContentDidUpdate(
        const base::DictValue& data) {}

   protected:
    virtual ~Observer() = default;
  };

  static void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry);
  static void MigrateObsoleteLocalStatePrefs(PrefService* local_state);

  NTPBackgroundImagesService(
      component_updater::ComponentUpdateService* component_update_service,
      sponsored_content::SponsoredContentService* sponsored_content_service,
      PrefService* pref_service);

  NTPBackgroundImagesService(const NTPBackgroundImagesService&) = delete;
  NTPBackgroundImagesService& operator=(const NTPBackgroundImagesService&) =
      delete;

  ~NTPBackgroundImagesService() override;

  void Init();
  void StartTearDown();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  bool HasObserver(Observer* observer);

  NTPBackgroundImagesData* GetBackgroundImagesData() const;
  NTPSponsoredContentData* GetNewTabTakeover(
      bool supports_dynamic_new_tab_takeover) const;
  NTPSponsoredSitesData* GetSponsoredSitesData() const;
  // Returns the absolute file path for a sponsored site image request path
  // (e.g. "amazon.png"), or nullopt if unavailable or invalid.
  std::optional<base::FilePath> MaybeGetSponsoredSiteImageFilePath(
      const base::FilePath& request_path) const;
  const std::optional<std::string>& GetSponsoredImagesComponentId() const;
  virtual void RegisterSponsoredImagesComponent();
  virtual void UnregisterSponsoredImagesComponent();

  // Each profile's `ViewCounterService` calls one of these when its opt-in
  // state changes. The component stays registered as long as at least one
  // loaded profile is opted in, so opting out in one profile does not clear
  // the sponsored images data still used by other loaded profiles.
  void AddSponsoredImagesOptedInProfile();
  void RemoveSponsoredImagesOptedInProfile();

  void MaybeCheckForSponsoredComponentUpdate();
  void ForceSponsoredComponentUpdate();

 private:
  friend class FakeNTPBackgroundImagesService;
  friend class NTPBackgroundImagesServiceForTesting;
  friend class NTPBackgroundImagesServiceTest;
  friend class ViewCounterServiceTest;
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesServiceTest, InternalDataTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesServiceTest,
                           MultipleCampaignsTest);
  FRIEND_TEST_ALL_PREFIXES(
      NTPBackgroundImagesServiceTest,
      DoNotGetSponsoredImageContentForNonHttpsSchemeTargetUrl);
  FRIEND_TEST_ALL_PREFIXES(
      NTPBackgroundImagesServiceTest,
      DoNotGetSponsoredImageContentIfWallpaperUrlReferencesParent);
  FRIEND_TEST_ALL_PREFIXES(
      NTPBackgroundImagesServiceTest,
      DoNotGetSponsoredImageContentIfWallpaperButtonImageRelativeUrlReferencesParent);
  FRIEND_TEST_ALL_PREFIXES(
      NTPBackgroundImagesServiceTest,
      DoNotGetSponsoredRichMediaContentIfWallpaperRelativeUrlReferencesParent);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesServiceTest,
                           SponsoredImageWithMissingImageUrlTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesServiceTest,
                           WithDefaultReferralCodeTest1);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesServiceTest,
                           WithDefaultReferralCodeTest2);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesServiceTest,
                           CheckReferralServiceInitStatusTest);
  FRIEND_TEST_ALL_PREFIXES(
      NTPBackgroundImagesServiceTest,
      CheckRecoverShutdownWhileMappingTableFetchingWithDefaultCode);
  FRIEND_TEST_ALL_PREFIXES(
      NTPBackgroundImagesServiceTest,
      CheckRecoverShutdownWhileMappingTableFetchingWithNonDefaultCode);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesSourceTest, BackgroundImagesTest);

  FRIEND_TEST_ALL_PREFIXES(
      ViewCounterServiceTest,
      AllowNewTabTakeoverWithRichMediaIfJavaScriptContentSettingIsSetToAllowed);
  FRIEND_TEST_ALL_PREFIXES(
      ViewCounterServiceTest,
      BlockNewTabTakeoverWithRichMediaIfJavaScriptContentSettingIsSetToBlocked);
  FRIEND_TEST_ALL_PREFIXES(
      ViewCounterServiceTest,
      AllowNewTabTakeoverWithImageIfJavaScriptContentSettingIsSetToAllowed);
  FRIEND_TEST_ALL_PREFIXES(
      ViewCounterServiceTest,
      AllowNewTabTakeoverWithImageIfJavaScriptContentSettingIsSetToBlocked);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest, ModelTest);

  void OnComponentReady(const base::FilePath& installed_dir);
  void OnGetComponentJsonData(const std::string& json_string);
  void OnVariationsCountryPrefChanged();

  void ResetSponsoredContentData();

  // SponsoredContentServiceObserver:
  void OnSponsoredNewTabTakeoverContentDidUpdate(
      const base::FilePath& installed_dir,
      const std::optional<base::DictValue>& dict) override;
  void OnSponsoredSiteContentDidUpdate(
      const base::FilePath& installed_dir,
      const std::optional<base::DictValue>& dict) override;
  void OnSponsoredConversionConfigContentDidUpdate(
      const base::FilePath& installed_dir,
      const std::optional<base::DictValue>& dict) override;

  // `FilterCampaigns` checks whether each creative's referenced files exist
  // on disk, so it must run off the UI thread.
  static base::DictValue FilterNewTabTakeoverCampaigns(
      base::DictValue dict,
      const base::FilePath& installed_dir);
  void FilterNewTabTakeoverCampaignsCallback(
      const base::FilePath& installed_dir,
      base::DictValue dict);

  // `NTPSponsoredSitesData`'s parsing checks whether each tile's image file
  // exists on disk, so it must run off the UI thread.
  static std::optional<NTPSponsoredSitesData> MaybeParseSponsoredSiteContent(
      base::DictValue dict,
      const base::FilePath& installed_dir,
      const std::string& url_prefix);
  void ParseSponsoredSiteContentCallback(
      std::optional<NTPSponsoredSitesData> sites_data);

  // virtual for test.
  virtual void RegisterBackgroundImagesComponent();

  raw_ptr<component_updater::ComponentUpdateService> component_update_service_ =
      nullptr;  // Not owned.

  // `StartTearDown` clears this on shutdown, so every use must null-check it.
  raw_ptr<sponsored_content::SponsoredContentService>
      sponsored_content_service_ = nullptr;  // Not owned.

  raw_ptr<PrefService> pref_service_ = nullptr;  // Not owned.
  PrefChangeRegistrar pref_change_registrar_;

  base::FilePath background_images_installed_dir_;
  std::unique_ptr<NTPBackgroundImagesData> background_images_data_;

  std::optional<base::FilePath> sponsored_content_installed_dir_;
  std::unique_ptr<NTPSponsoredContentData> sponsored_content_data_;
  std::unique_ptr<NTPSponsoredContentData>
      sponsored_content_data_excluding_dynamic_;
  std::unique_ptr<NTPSponsoredSitesData> sponsored_sites_data_;

  size_t sponsored_images_opted_in_profile_count_ = 0;

  base::ObserverList<Observer>::Unchecked observers_;

  base::WeakPtrFactory<NTPBackgroundImagesService> weak_factory_{this};
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_H_
