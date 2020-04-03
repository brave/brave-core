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

namespace content {
class WebUIDataSource;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // network

class PrefService;

namespace ntp_background_images {

struct NTPBackgroundImagesData;

class NTPBackgroundImagesService {
 public:
  class Observer {
   public:
    // Called whenever ntp background images component is updated.
    virtual void OnUpdated(NTPBackgroundImagesData* data) = 0;
   protected:
    virtual ~Observer() {}
  };

  NTPBackgroundImagesService(
      component_updater::ComponentUpdateService* cus,
      PrefService* local_pref,
      const base::FilePath& user_data_dir,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
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

  void InitializeWebUIDataSource(content::WebUIDataSource* html_source);

  // This api can be used for fast checking before SR component registration.
  // NOTE: SR Data could not be availble even if this returns true because
  // component data loading could not be finished yet.
  // Use this api just for checking whether this install is SR.
  // This returns true when we certainly know this install is SR.
  // If this returns false, we don't know this install is SR or not for now.
  // So, don't assume this install is not SR if this returns false.
  bool IsSuperReferral() const;
  std::vector<std::string> GetCachedTopSitesFaviconList() const;

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
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesSourceTest, BasicTest);

  void OnComponentReady(bool is_super_referral,
                        const base::FilePath& installed_dir);
  void OnGetComponentJsonData(bool is_super_referral,
                              const std::string& json_string);
  void OnPreferenceChanged(const std::string& pref_name);
  void OnGetMappingTableData(std::unique_ptr<std::string> json_string);

  std::string GetReferralPromoCode() const;

  void CacheTopSitesFaviconList();
  void RestoreCachedTopSitesFaviconList();

  void ScheduleMappingTabRetryTimer();

  // Return true if test data is passed.
  bool UseLocalSponsoredImagesestData();
  bool UseLocalSuperReferralTestData();
  void RegisterDemoSuperReferralComponent();

  // virtual for test.
  virtual void CheckSuperReferralComponent();
  virtual void RegisterSponsoredImagesComponent();
  virtual void RegisterSuperReferralComponent();
  virtual void DownloadSuperReferralMappingTable();
  virtual void MonitorReferralPromoCodeChange();
  virtual void UnRegisterSuperReferralComponent();
  virtual void MarkThisInstallIsNotSuperReferralForever();

  std::vector<std::string> cached_top_site_favicon_list_;
  bool test_data_used_ = false;
  component_updater::ComponentUpdateService* component_update_service_;
  PrefService* local_pref_;
  const base::FilePath top_sites_favicon_cache_dir_;
  base::FilePath si_installed_dir_;
  base::FilePath sr_installed_dir_;
  base::ObserverList<Observer>::Unchecked observer_list_;
  std::unique_ptr<NTPBackgroundImagesData> si_images_data_;
  std::unique_ptr<NTPBackgroundImagesData> sr_images_data_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> loader_;
  PrefChangeRegistrar pref_change_registrar_;
  std::unique_ptr<base::OneShotTimer> mapping_table_retry_timer_;
  base::WeakPtrFactory<NTPBackgroundImagesService> weak_factory_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_H_
