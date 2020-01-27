/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_SERVICE_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_SERVICE_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"

FORWARD_DECLARE_TEST(NTPSponsoredImagesServiceTest, InternalDataTest);

namespace component_updater {
class ComponentUpdateService;
}

namespace content {
class BrowserContext;
}  // namespace content

class NTPSponsoredImagesServiceTest;

namespace ntp_sponsored_images {

struct NTPSponsoredImagesData;

class NTPSponsoredImagesService {
 public:
  class Observer {
   public:
    // Called whenever ntp sponsored images component is updated.
    virtual void OnUpdated(NTPSponsoredImagesData* data) = 0;
   protected:
    virtual ~Observer() {}
  };

  explicit NTPSponsoredImagesService(
      component_updater::ComponentUpdateService* cus);
  ~NTPSponsoredImagesService();

  NTPSponsoredImagesService(const NTPSponsoredImagesService&) = delete;
  NTPSponsoredImagesService& operator=(
      const NTPSponsoredImagesService&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  bool HasObserver(Observer* observer);

  // This should be called by client because this service is context neutral.
  void AddDataSource(content::BrowserContext* browser_context);

  NTPSponsoredImagesData* GetSponsoredImagesData() const;

 private:
  FRIEND_TEST_ALL_PREFIXES(::NTPSponsoredImagesServiceTest, InternalDataTest);

  void OnComponentReady(const base::FilePath& installed_dir);
  void OnGetPhotoJsonData(const std::string& photo_json);
  void NotifyObservers();

  void ResetImagesDataForTest();

  base::FilePath photos_manifest_path_;
  base::ObserverList<Observer>::Unchecked observer_list_;
  std::unique_ptr<NTPSponsoredImagesData> images_data_;
  base::WeakPtrFactory<NTPSponsoredImagesService> weak_factory_;
};

}  // namespace ntp_sponsored_images
#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_SERVICE_H_
