/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_COMBINED_UTILITY_BROWSER_COMBINED_UTILITY_SERVICE_H_
#define BRAVE_COMPONENTS_COMBINED_UTILITY_BROWSER_COMBINED_UTILITY_SERVICE_H_

#include <memory>
#include <utility>

#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#include "brave/components/services/combined_utility/public/interfaces/combined_utility.mojom.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace combined_utility {

class ServiceInstance : public base::RefCounted<ServiceInstance>,
                        public base::SupportsWeakPtr<ServiceInstance> {
 public:
  ServiceInstance(mojo::Remote<combined_utility::mojom::BatAdsLedgerFactory>
                      interface_remote);

  mojo::Remote<combined_utility::mojom::BatAdsLedgerFactory>&
  interface_remote() {
    return interface_remote_;
  }

 private:
  friend class base::RefCounted<ServiceInstance>;

  ~ServiceInstance();

  mojo::Remote<combined_utility::mojom::BatAdsLedgerFactory> interface_remote_;
};

template <class T>
class InterfaceHolder {
 public:
  InterfaceHolder() = default;
  InterfaceHolder(mojo::Remote<T> interface_remote,
                  scoped_refptr<ServiceInstance> instance)
      : interface_remote_(std::move(interface_remote)),
        instance_(std::move(instance)) {}

  mojo::Remote<T>& Get() { return interface_remote_; }
  void Reset() {
    instance_.reset();
    interface_remote_.reset();
  }

 private:
  mojo::Remote<T> interface_remote_;
  scoped_refptr<ServiceInstance> instance_;
};

class CombinedUtilityService : public KeyedService {
 public:
  explicit CombinedUtilityService(content::BrowserContext* browser_context);

  InterfaceHolder<bat_ledger::mojom::BatLedgerService> MakeBatLedgerService();
  InterfaceHolder<bat_ads::mojom::BatAdsService> MakeBatAdsService();

 private:
  ~CombinedUtilityService() override;

  scoped_refptr<ServiceInstance> MakeStrongBatAdsLedgerFactory();

  base::WeakPtr<ServiceInstance> bat_ads_ledger_factory_weak_;
};

class CombinedUtilityServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static CombinedUtilityService* GetForBrowserContext(
      content::BrowserContext* browser_context);

  static CombinedUtilityServiceFactory* GetInstance();

  CombinedUtilityServiceFactory();

 private:
  friend struct base::DefaultSingletonTraits<CombinedUtilityServiceFactory>;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

}  // namespace combined_utility
#endif  // BRAVE_COMPONENTS_COMBINED_UTILITY_BROWSER_COMBINED_UTILITY_SERVICE_H_
