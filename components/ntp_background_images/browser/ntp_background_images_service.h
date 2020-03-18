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

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace ntp_background_images {

struct NTPBackgroundImagesData;

class NTPBackgroundImagesService {
 public:
  class Observer {
   public:
    // Called whenever ntp sponsored images component is updated.
    virtual void OnUpdated(NTPBackgroundImagesData* data) = 0;
   protected:
    virtual ~Observer() {}
  };

  explicit NTPBackgroundImagesService(
      component_updater::ComponentUpdateService* cus);
  ~NTPBackgroundImagesService();

  NTPBackgroundImagesService(const NTPBackgroundImagesService&) = delete;
  NTPBackgroundImagesService& operator=(
      const NTPBackgroundImagesService&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  bool HasObserver(Observer* observer);

  NTPBackgroundImagesData* GetBackgroundImagesData() const;

 private:
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesServiceTest, InternalDataTest);
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

  void OnComponentReady(const base::FilePath& installed_dir);
  void OnGetPhotoJsonData(const std::string& photo_json);
  void NotifyObservers();

  base::FilePath installed_dir_;
  base::ObserverList<Observer>::Unchecked observer_list_;
  std::unique_ptr<NTPBackgroundImagesData> images_data_;
  base::WeakPtrFactory<NTPBackgroundImagesService> weak_factory_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_H_
