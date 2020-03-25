/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
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
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  virtual ~NTPBackgroundImagesService();

  NTPBackgroundImagesService(const NTPBackgroundImagesService&) = delete;
  NTPBackgroundImagesService& operator=(
      const NTPBackgroundImagesService&) = delete;

  void Init();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  bool HasObserver(Observer* observer);

  NTPBackgroundImagesData* GetBackgroundImagesData() const;

  bool test_data_used() const { return test_data_used_; }

  void InitializeWebUIDataSource(content::WebUIDataSource* html_source);

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
                           InstallSuperReferralOverReferralTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesServiceTest,
                           SimulateNetworkErrorTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesServiceTest,
                           BasicSuperReferralTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesServiceTest, CleanUpTest);
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
  void OnGetComponentJsonData(const std::string& json_string);
  void OnPreferenceChanged(const std::string& pref_name);
  void OnGetMappingTableData(std::unique_ptr<std::string> json_string);

  void GetComponentJsonData(const base::FilePath& installed_dir);
  void NotifyObservers();
  void DetermineTargetComponent();
  bool IsSuperReferralCode(const std::string& referral_code);
  std::string GetReferralPromoCode() const;
  std::string GetCachedReferralPromoCode() const;
  void UnRegisterSponsoredImagesComponentIfRunning();
  bool IsAlreadyRegistered(const std::string& component_id);

  // Returns true if local test data is passed via command line.
  bool UseLocalTestData();

  void CleanUp();

  // Check super referral component is available for this install and start
  // start it if we can confirm this insall comes from super referral.
  // Otherwise, start sponsored images component.
  // virtual for test.
  virtual void StartSuperReferralComponent(
      const std::string& super_referral_code);
  virtual void StartSponsoredImagesComponent();
  virtual void DownloadSuperReferralMappingTable();
  virtual void MonitorReferralPromoCodeChange();
  virtual void UnRegisterSuperReferralComponentIfRunning(
      const std::string& referral_code);

  // Used to flag what component is our last decision.
  // If this is true, NTP SR component registration is requested lastly.
  bool is_super_referral_lastly_asked_component_ = false;
  bool test_data_used_ = false;
  base::Value mapping_table_value_;
  component_updater::ComponentUpdateService* component_update_service_;
  PrefService* local_pref_;
  base::FilePath installed_dir_;
  base::ObserverList<Observer>::Unchecked observer_list_;
  std::unique_ptr<NTPBackgroundImagesData> images_data_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> loader_;
  PrefChangeRegistrar pref_change_registrar_;
  bool enable_super_referral_for_test_ = false;
  base::WeakPtrFactory<NTPBackgroundImagesService> weak_factory_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_H_
