// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_REFERRAL_IMAGES_SERVICE_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_REFERRAL_IMAGES_SERVICE_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/values.h"
#include "components/prefs/pref_change_registrar.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

class PrefRegistrySimple;
class PrefService;

namespace ntp_sponsored_images {

struct NTPReferralImagesData;

class NTPReferralImagesService {
 public:
  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  class Observer {
   public:
    // Called whenever ntp referral images component is updated.
    virtual void OnReferralImagesUpdated(NTPReferralImagesData* data) = 0;
   protected:
    virtual ~Observer() {}
  };

  NTPReferralImagesService(component_updater::ComponentUpdateService* cus,
                           PrefService* local_pref);
  virtual ~NTPReferralImagesService();

  NTPReferralImagesService(const NTPReferralImagesService&) = delete;
  NTPReferralImagesService& operator=(
      const NTPReferralImagesService&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  bool HasObserver(Observer* observer);

  // Return nullptr if this install is not from super referrer.
  // Otherwise, this should not be nullptr but can be Invalid data.
  // That means, super referral image is not ready.
  NTPReferralImagesData* GetReferralImagesData() const;

  bool is_super_referral() const { return is_super_referral_; }

 private:
  friend class NTPReferralImagesServiceTest;
  FRIEND_TEST_ALL_PREFIXES(NTPReferralImagesServiceTest, ImageSourceTest);
  FRIEND_TEST_ALL_PREFIXES(NTPReferralImagesServiceTest, InternalDataTest);
  FRIEND_TEST_ALL_PREFIXES(NTPReferralImagesServiceTest, MapperComponentTest);

  void OnPreferenceChanged();

  // Called when referrer component that has super referrer's assets is ready.
  void OnReferralComponentReady(const base::FilePath& installed_dir);
  void OnGetReferralJsonData(const base::FilePath& installed_dir,
                             const std::string& json);
  void NotifyObservers();

  // Called when mapper component is ready.
  void OnMapperComponentReady(const base::FilePath& installed_dir);
  void OnGetMappingJsonData(const std::string& json);
  // virtual for test.
  virtual void RegisterReferralComponent();

  // This will be true until we can confirm that this is not super referral
  // install.
  bool is_super_referral_ = true;
  component_updater::ComponentUpdateService* cus_;
  PrefService* local_pref_;
  PrefChangeRegistrar pref_change_registrar_;
  base::ObserverList<Observer>::Unchecked observer_list_;
  std::unique_ptr<NTPReferralImagesData> images_data_;
  base::WeakPtrFactory<NTPReferralImagesService> weak_factory_;
};

}  // namespace ntp_sponsored_images


#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_REFERRAL_IMAGES_SERVICE_H_
