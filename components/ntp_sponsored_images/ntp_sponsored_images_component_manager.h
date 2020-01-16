/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGES_COMPONENT_MANAGER_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGES_COMPONENT_MANAGER_H_

#include <memory>
#include <string>

#include "base/component_export.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/ntp_sponsored_images/ntp_sponsored_image_source.h"
#include "components/component_updater/component_updater_service.h"

struct NTPSponsoredImagesData;
struct NTPSponsoredImagesInternalData;

namespace content {
class BrowserContext;
}  // namespace content

// NOTE: Client should call AddDataSource() before requesting image sources.
// When component is updated, Client also should call AddDataSource() for add
// proper |URLDataSource|s.
class COMPONENT_EXPORT(BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES)
NTPSponsoredImagesComponentManager
    : public brave_component_updater::BraveComponent,
      public component_updater::ServiceObserver  {
 public:
  class Observer {
   public:
    // Called whenever ntp sponsored images component is updated.
    virtual void OnUpdated(const NTPSponsoredImagesData& data) = 0;
   protected:
    virtual ~Observer() {}
  };

  NTPSponsoredImagesComponentManager(
      BraveComponent::Delegate* delegate,
      component_updater::ComponentUpdateService* cus);
  ~NTPSponsoredImagesComponentManager() override;

  NTPSponsoredImagesComponentManager(
      const NTPSponsoredImagesComponentManager&) = delete;
  NTPSponsoredImagesComponentManager& operator=(
      const NTPSponsoredImagesComponentManager&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // This should be called by client because this service is context neutral.
  void AddDataSource(content::BrowserContext* browser_context);

  base::Optional<NTPSponsoredImagesData> GetLatestSponsoredImagesData() const;

 private:
  FRIEND_TEST_ALL_PREFIXES(NTPSponsoredImagesComponentManagerTest,
                           InternalDataTest);

  // BraveComponent overrides:
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& installed_dir,
                        const std::string& manifest) override;

  // ServiceObserver overrides:
  void OnEvent(Events event, const std::string& id) override;

  void ReadPhotoJsonFileAndNotify();
  void OnGetPhotoJsonData(const std::string& photo_json);
  void ParseAndCachePhotoJsonData(const std::string& photo_json);
  void NotifyObservers();

  void ResetInternalImagesDataForTest();

  base::FilePath photo_json_file_path_;
  component_updater::ComponentUpdateService* cus_ = nullptr;
  base::ObserverList<Observer>::Unchecked observer_list_;
  std::unique_ptr<NTPSponsoredImagesInternalData> internal_images_data_;
  base::WeakPtrFactory<NTPSponsoredImagesComponentManager> weak_factory_;
};

#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGES_COMPONENT_MANAGER_H_
