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
#include "base/timer/wall_clock_timer.h"
#include "base/values.h"
#include "components/prefs/pref_change_registrar.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace variations {
class VariationsService;
}  // namespace variations

class PrefRegistrySimple;
class PrefService;

namespace ntp_background_images {

struct NTPBackgroundImagesData;
struct NTPSponsoredImagesData;

class NTPBackgroundImagesService {
 public:
  class Observer {
   public:
    // Called when the background images component is updated.
    virtual void OnBackgroundImagesDataDidUpdate(
        NTPBackgroundImagesData* data) {}

    // Called when the sponsored content component is updated. This is
    // deprecated, use `OnSponsoredContentDidUpdate`.
    virtual void OnSponsoredImagesDataDidUpdate(NTPSponsoredImagesData* data) {}

    // Called when the sponsored content component is updated.
    virtual void OnSponsoredContentDidUpdate(const base::Value::Dict& data) {}

   protected:
    virtual ~Observer() = default;
  };

  static void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry);
  static void MigrateObsoleteLocalStatePrefs(PrefService* local_state);

  NTPBackgroundImagesService(
      variations::VariationsService* variations_service,
      component_updater::ComponentUpdateService* component_update_service,
      PrefService* pref_service);
  virtual ~NTPBackgroundImagesService();

  NTPBackgroundImagesService(const NTPBackgroundImagesService&) = delete;
  NTPBackgroundImagesService& operator=(
      const NTPBackgroundImagesService&) = delete;

  void Init();
  void StartTearDown();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  bool HasObserver(Observer* observer);

  NTPBackgroundImagesData* GetBackgroundImagesData() const;
  NTPSponsoredImagesData* GetSponsoredImagesData(
      bool supports_rich_media) const;
  virtual void RegisterSponsoredImagesComponent();

  void MaybeCheckForSponsoredComponentUpdate();
  void ForceSponsoredComponentUpdate();

 private:
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

  void OnSponsoredComponentReady(const base::FilePath& installed_dir);
  void OnGetSponsoredComponentJsonData(const std::string& json_string);
  void OnComponentReady(const base::FilePath& installed_dir);
  void OnGetComponentJsonData(const std::string& json_string);
  void OnVariationsCountryPrefChanged();

  void ScheduleNextSponsoredImagesComponentUpdate();
  void CheckSponsoredImagesComponentUpdate(const std::string& component_id);

  // virtual for test.
  virtual void RegisterBackgroundImagesComponent();

  std::optional<base::Time> last_updated_at_;

  // `variations` can be null in test.
  raw_ptr<variations::VariationsService> variations_service_ =
      nullptr;  // Not owned.

  raw_ptr<component_updater::ComponentUpdateService> component_update_service_ =
      nullptr;

  raw_ptr<PrefService> pref_service_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;

  base::FilePath background_images_installed_dir_;
  std::unique_ptr<NTPBackgroundImagesData> background_images_data_;

  base::WallClockTimer sponsored_images_update_check_timer_;
  base::RepeatingClosure sponsored_images_update_check_callback_;
  std::optional<std::string> sponsored_images_component_id_;
  base::FilePath sponsored_images_installed_dir_;
  std::unique_ptr<NTPSponsoredImagesData> sponsored_images_data_;
  std::unique_ptr<NTPSponsoredImagesData>
      sponsored_images_data_excluding_rich_media_;

  base::ObserverList<Observer>::Unchecked observers_;

  base::WeakPtrFactory<NTPBackgroundImagesService> weak_factory_{this};
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_H_
