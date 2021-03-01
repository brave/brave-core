/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "components/prefs/pref_change_registrar.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

class PrefRegistrySimple;
class PrefService;

namespace ntp_background_images {

struct NTPBackgroundImagesData;

class NTPBackgroundImagesService {
 public:
  class Observer {
   public:
    // Called whenever ntp background images component is updated.
    virtual void OnUpdated(NTPBackgroundImagesData* data) = 0;
    // Called when SR campaign ended.
    virtual void OnSuperReferralEnded() = 0;
   protected:
    virtual ~Observer() {}
  };

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  NTPBackgroundImagesService(
      component_updater::ComponentUpdateService* cus,
      PrefService* local_pref);
  virtual ~NTPBackgroundImagesService();

  NTPBackgroundImagesService(const NTPBackgroundImagesService&) = delete;
  NTPBackgroundImagesService& operator=(
      const NTPBackgroundImagesService&) = delete;

  void Init();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  bool HasObserver(Observer* observer);

  NTPBackgroundImagesData* GetBackgroundImagesData(bool super_referral) const;

  bool test_data_used() const { return test_data_used_; }

  bool IsSuperReferral() const;
  std::string GetSuperReferralThemeName() const;
  std::string GetSuperReferralCode() const;

  std::vector<std::string> GetTopSitesFaviconList() const;

 private:
  friend class TestNTPBackgroundImagesService;
  friend class NTPBackgroundImagesServiceTest;
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesServiceTest, InternalDataTest);
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
                           ActiveOptedInWithNTPBackgroundOption);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           NotActiveInitially);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           NotActiveWithBadData);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           NotActiveOptedOut);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           IsActiveOptedIn);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           ActiveInitiallyOptedIn);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest, ModelTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesSourceTest, BasicTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesSourceTest,
                           BasicSuperReferralDataTest);

  void OnComponentReady(bool is_super_referral,
                        const base::FilePath& installed_dir);
  void OnGetComponentJsonData(bool is_super_referral,
                              const std::string& json_string);
  void OnMappingTableComponentReady(const base::FilePath& installed_dir);
  void OnPreferenceChanged(const std::string& pref_name);
  void OnGetMappingTableData(const std::string& json_string);

  std::string GetReferralPromoCode() const;
  bool IsValidSuperReferralComponentInfo(
      const base::Value& component_info) const;

  void CacheTopSitesFaviconList();
  void CheckSIComponentUpdate(const std::string& component_id);

  // virtual for test.
  virtual void CheckSuperReferralComponent();
  virtual void RegisterSponsoredImagesComponent();
  virtual void RegisterSuperReferralComponent();
  virtual void DownloadSuperReferralMappingTable();
  virtual void MonitorReferralPromoCodeChange();
  virtual void UnRegisterSuperReferralComponent();
  virtual void MarkThisInstallIsNotSuperReferralForever();

  base::RepeatingTimer si_update_check_timer_;
  std::vector<std::string> top_site_favicon_list_;
  bool test_data_used_ = false;
  component_updater::ComponentUpdateService* component_update_service_;
  PrefService* local_pref_;
  base::FilePath si_installed_dir_;
  base::FilePath sr_installed_dir_;
  base::ObserverList<Observer>::Unchecked observer_list_;
  std::unique_ptr<NTPBackgroundImagesData> si_images_data_;
  std::unique_ptr<NTPBackgroundImagesData> sr_images_data_;
  PrefChangeRegistrar pref_change_registrar_;
  // This is only used for registration during initial(first) SR component
  // download. After initial download is done, it's cached to
  // |kNewTabPageCachedSuperReferralComponentInfo|. At next launch, this cached
  // info is used for registering SR component.
  // Why component info is temporarily stored to |initial_sr_component_info_|
  // when mapping table is fetched instead of directly store it into that prefs?
  // The reason is |kNewTabPageCachedSuperReferralComponentInfo| is used to
  // check whether initial download is finished or not. Knowing initial download
  // is done is important for super referral. If this is SR install, we should
  // not show SI images until user chooses Brave default images. So, we should
  // know the exact timing whether SR assets is ready to use or not.
  base::Value initial_sr_component_info_;
  base::WeakPtrFactory<NTPBackgroundImagesService> weak_factory_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_H_
