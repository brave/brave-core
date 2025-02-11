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

    // Called when the super referral campaign ends.
    virtual void OnSuperReferralCampaignDidEnd() {}

   protected:
    virtual ~Observer() = default;
  };

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  NTPBackgroundImagesService(
      component_updater::ComponentUpdateService* component_update_service,
      PrefService* pref_service);
  virtual ~NTPBackgroundImagesService();

  NTPBackgroundImagesService(const NTPBackgroundImagesService&) = delete;
  NTPBackgroundImagesService& operator=(
      const NTPBackgroundImagesService&) = delete;

  void Init();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  bool HasObserver(Observer* observer);

  NTPBackgroundImagesData* GetBackgroundImagesData() const;
  NTPSponsoredImagesData* GetBrandedImagesData(bool super_referral) const;

  bool test_data_used() const { return test_data_used_; }

  bool IsSuperReferral() const;
  std::string GetSuperReferralThemeName() const;
  std::string GetSuperReferralCode() const;

  void MaybeCheckForSponsoredComponentUpdate();

 private:
  friend class NTPSponsoredRichMediaSourceTest;
  friend class NTPSponsoredRichMediaWithCSPViolationBrowserTest;
  friend class NTPSponsoredRichMediaBrowserTest;
  friend class NTPBackgroundImagesServiceForTesting;
  friend class NTPBackgroundImagesServiceTest;
  friend class NTPBackgroundImagesViewCounterTest;
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
                           WithNonSuperReferralCodeTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesServiceTest,
                           WithSuperReferralCodeTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesServiceTest,
                           BasicSuperReferralTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesServiceTest,
                           CheckReferralServiceInitStatusTest);
  FRIEND_TEST_ALL_PREFIXES(
      NTPBackgroundImagesServiceTest,
      CheckRecoverShutdownWhileMappingTableFetchingWithDefaultCode);
  FRIEND_TEST_ALL_PREFIXES(
      NTPBackgroundImagesServiceTest,
      CheckRecoverShutdownWhileMappingTableFetchingWithNonDefaultCode);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           SINotActiveInitially);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           SINotActiveWithBadData);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           NotActiveOptedOut);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           IsActiveOptedIn);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           ActiveInitiallyOptedIn);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           ActiveOptedInWithNTPBackgoundOption);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest, ModelTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           BINotActiveInitially);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           BINotActiveWithBadData);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           BINotActiveWithNTPBackgoundOptionOptedOut);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesSourceTest, SponsoredImagesTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesSourceTest,
                           BasicSuperReferralDataTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesSourceTest, BackgroundImagesTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           GetCurrentWallpaperTest);

  void OnSponsoredComponentReady(bool is_super_referral,
                                 const base::FilePath& installed_dir);
  void OnGetSponsoredComponentJsonData(bool is_super_referral,
                                       const std::string& json_string);
  void OnComponentReady(const base::FilePath& installed_dir);
  void OnGetComponentJsonData(const std::string& json_string);
  void OnMappingTableComponentReady(const base::FilePath& installed_dir);
  void OnPreferenceChanged(const std::string& pref_name);
  void OnCountryCodePrefChanged();
  void OnGetMappingTableData(const std::string& json_string);

  std::string GetReferralPromoCode() const;
  bool IsValidSuperReferralComponentInfo(
      const base::Value::Dict& component_info) const;

  void ScheduleNextSponsoredImagesComponentUpdate();
  void CheckSponsoredImagesComponentUpdate(const std::string& component_id);

  // virtual for test.
  virtual void CheckSuperReferralComponent();
  virtual void RegisterBackgroundImagesComponent();
  virtual void RegisterSponsoredImagesComponent();
  virtual void RegisterSuperReferralComponent();
  virtual void DownloadSuperReferralMappingTable();
  virtual void MonitorReferralPromoCodeChange();
  virtual void UnRegisterSuperReferralComponent();
  virtual void MarkThisInstallIsNotSuperReferralForever();

  base::Time last_update_check_at_;

  bool test_data_used_ = false;

  const raw_ptr<component_updater::ComponentUpdateService>
      component_update_service_ = nullptr;

  const raw_ptr<PrefService> pref_service_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;

  base::FilePath background_images_installed_dir_;
  std::unique_ptr<NTPBackgroundImagesData> background_images_data_;

  base::WallClockTimer sponsored_images_update_check_timer_;
  base::RepeatingClosure sponsored_images_update_check_callback_;
  std::optional<std::string> sponsored_images_component_id_;
  base::FilePath sponsored_images_installed_dir_;
  std::unique_ptr<NTPSponsoredImagesData> sponsored_images_data_;

  base::FilePath super_referrals_installed_dir_;
  std::unique_ptr<NTPSponsoredImagesData> super_referrals_images_data_;
  // This is only used for registration during initial(first) SR component
  // download. After initial download is done, it's cached to
  // |kNewTabPageCachedSuperReferralComponentInfo|. At next launch, this cached
  // info is used for registering SR component. Why component info is
  // temporarily stored to |initial_super_referrals_component_info_| when
  // mapping table is fetched instead of directly store it into that prefs? The
  // reason is |kNewTabPageCachedSuperReferralComponentInfo| is used to check
  // whether initial download is finished or not. Knowing initial download is
  // done is important for super referral. If this is SR install, we should not
  // show SI images until user chooses Brave default images. So, we should know
  // the exact timing whether SR assets is ready to use or not.
  std::optional<base::Value::Dict> initial_super_referrals_component_info_;

  base::ObserverList<Observer>::Unchecked observers_;

  base::WeakPtrFactory<NTPBackgroundImagesService> weak_factory_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_H_
