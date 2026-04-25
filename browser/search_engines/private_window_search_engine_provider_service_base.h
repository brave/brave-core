/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINES_PRIVATE_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_BASE_H_
#define BRAVE_BROWSER_SEARCH_ENGINES_PRIVATE_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_BASE_H_

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;
class TemplateURL;
class TemplateURLService;

class PrivateWindowSearchEngineProviderServiceBase : public KeyedService {
 public:
  explicit PrivateWindowSearchEngineProviderServiceBase(Profile* otr_profile);
  ~PrivateWindowSearchEngineProviderServiceBase() override;

  PrivateWindowSearchEngineProviderServiceBase(
      const PrivateWindowSearchEngineProviderServiceBase&) = delete;
  PrivateWindowSearchEngineProviderServiceBase& operator=(
      const PrivateWindowSearchEngineProviderServiceBase&) = delete;

 protected:
  // KeyedService overrides:
  void Shutdown() override;

  virtual void Initialize() {}
  bool ShouldUseExtensionSearchProvider() const;
  void UseExtensionSearchProvider();

  // Points off the record profile.
  raw_ptr<Profile> otr_profile_ = nullptr;
  // Service for original profile of |otr_profile_|.
  raw_ptr<TemplateURLService> original_template_url_service_ = nullptr;
  // Service for off the record profile.
  raw_ptr<TemplateURLService> otr_template_url_service_ = nullptr;

 private:
  bool CouldAddExtensionTemplateURL(const TemplateURL* url);
  void OnTemplateURLServiceLoaded();

  base::CallbackListSubscription template_url_service_subscription_;
  base::WeakPtrFactory<PrivateWindowSearchEngineProviderServiceBase>
      weak_factory_{this};
};

#endif  // BRAVE_BROWSER_SEARCH_ENGINES_PRIVATE_WINDOW_SEARCH_ENGINE_PROVIDER_SERVICE_BASE_H_
