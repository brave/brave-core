/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/private_window_search_engine_provider_service_base.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_service.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_prefs.h"
#endif

PrivateWindowSearchEngineProviderServiceBase::
    PrivateWindowSearchEngineProviderServiceBase(Profile* otr_profile)
    : otr_profile_(otr_profile),
      original_template_url_service_(TemplateURLServiceFactory::GetForProfile(
          otr_profile_->GetOriginalProfile())),
      otr_template_url_service_(
          TemplateURLServiceFactory::GetForProfile(otr_profile_)) {
  if (original_template_url_service_->loaded()) {
    // Avoid calling virtual method from ctor.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &PrivateWindowSearchEngineProviderServiceBase::Initialize,
            weak_factory_.GetWeakPtr()));
    return;
  }

  // Using Unretained safe with subscription_.
  template_url_service_subscription_ =
      original_template_url_service_->RegisterOnLoadedCallback(
          base::BindOnce(&PrivateWindowSearchEngineProviderServiceBase::
                             OnTemplateURLServiceLoaded,
                         base::Unretained(this)));
}

PrivateWindowSearchEngineProviderServiceBase::
    ~PrivateWindowSearchEngineProviderServiceBase() = default;

void PrivateWindowSearchEngineProviderServiceBase::
    UseExtensionSearchProvider() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  DCHECK(ShouldUseExtensionSearchProvider());

  const auto* extension_provider_url =
      original_template_url_service_->GetDefaultSearchProvider();
  if (!extension_provider_url)
    return;
  auto data = extension_provider_url->data();
  data.id = kInvalidTemplateURLID;

  // Can't add same turl again to service.
  if (CouldAddExtensionTemplateURL(extension_provider_url)) {
    auto type = extension_provider_url->type();
    auto extension_id = extension_provider_url->GetExtensionId();
    extensions::ExtensionPrefs* prefs =
        extensions::ExtensionPrefs::Get(otr_profile_->GetOriginalProfile());
    auto time = prefs->GetLastUpdateTime(extension_id);

    auto turl =
        std::make_unique<TemplateURL>(data, type, extension_id, time, true);

    otr_template_url_service_->Add(std::move(turl));
  }

  // Clear default provider's guid to prevent unnecessary
  // |kDefaultSearchProviderDataPrefName| update when search provider's favicon
  // url is updated. If this is not cleared,  previous non-extension default
  // search provider's guid is stored.
  // For example, TemplateURLService::MaybeUpdateDSEViaPrefs() could update
  // |kDefaultSearchProviderDataPrefName| if previous default search
  // provider is qwant and current one is from extension and user searches
  // with qwant keyword(:q) and it's favicon url is updated.
  // Why we should prevent this? If not prevented, this user pref is updated.
  // Then, this update pref's data is loaded again by
  // DefaultSearchManager::LoadDefaultSearchEngineFromPrefs().
  // Here, we have different behavior from upstream.
  // In upstream, DSM still get extension's provider data from
  // |pref_service_->GetDictionary(kDefaultSearchProviderDataPrefName)| because
  // extension controlled prefs has more higher priority than user set pref.
  // So, still DefaultSearchManager::extension_default_search_| stores extension
  // search provider.
  // Howeveer, we manually set |kDefaultSearchProviderDataPrefName|. So,
  // It's replaced with qwant provider. It was extension search provider.
  // This only happens when |kSyncedDefaultSearchProviderGUID| and favicon
  // udpated search provider is same. (See the condition in
  // TemplateURLService::MaybeUpdateDSEViaPrefs())
  // And there is no side-effect when this prefs update is skipped because
  // it can be updated again when qwant is default search provider.
  otr_profile_->GetPrefs()->SetString(prefs::kSyncedDefaultSearchProviderGUID,
                                      data.sync_guid);

  otr_profile_->GetPrefs()->SetDict(
      DefaultSearchManager::kDefaultSearchProviderDataPrefName,
      TemplateURLDataToDictionary(data));
#endif
}

bool PrivateWindowSearchEngineProviderServiceBase::
    ShouldUseExtensionSearchProvider() const {
  return original_template_url_service_->IsExtensionControlledDefaultSearch();
}

bool PrivateWindowSearchEngineProviderServiceBase::CouldAddExtensionTemplateURL(
    const TemplateURL* url) {
  DCHECK(url);
  DCHECK_NE(TemplateURL::NORMAL, url->type());
  for (const TemplateURL* turl : otr_template_url_service_->GetTemplateURLs()) {
    DCHECK(turl);
    if (url->type() == turl->type() &&
        url->GetExtensionId() == turl->GetExtensionId())
      return false;
  }
  return true;
}

void PrivateWindowSearchEngineProviderServiceBase::
    OnTemplateURLServiceLoaded() {
  template_url_service_subscription_ = {};
  Initialize();
}

void PrivateWindowSearchEngineProviderServiceBase::Shutdown() {
  template_url_service_subscription_ = {};
}
