/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_SERVICE_IMPL_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace bat_ads {

class BatAdsServiceImpl : public mojom::BatAdsService {
 public:
  explicit BatAdsServiceImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~BatAdsServiceImpl() override;

  // Overridden from BatAdsService:
  void Create(mojom::BatAdsClientAssociatedPtrInfo client_info,
              mojom::BatAdsAssociatedRequest bat_ads,
              CreateCallback callback) override;

  // void Destroy(filesystem::mojom::DirectoryPtr directory,
  //              const std::string& dbname,
  //              DestroyCallback callback) override;

 private:
  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(BatAdsServiceImpl);
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_SERVICE_IMPL_H_
