/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engine_provider_controller_base.h"

#include "brave/common/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/prepopulated_engines.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"

class SearchEngineProviderControllerBase::Destroyer
    : public TemplateURLServiceObserver {
 public:
  Destroyer(SearchEngineProviderControllerBase* controller,
            TemplateURLService* otr_service)
      : controller_(controller),
        otr_service_(otr_service) {
    otr_service_->AddObserver(this);
  }
  ~Destroyer() override {}

 private:
  // TemplateURLServiceObserver overrides:
  void OnTemplateURLServiceChanged() override {}
  void OnTemplateURLServiceShuttingDown() override {
    otr_service_->RemoveObserver(this);
    delete controller_;
    delete this;
  }

  SearchEngineProviderControllerBase* controller_;
  TemplateURLService* otr_service_;

  DISALLOW_COPY_AND_ASSIGN(Destroyer);
};

SearchEngineProviderControllerBase::SearchEngineProviderControllerBase(
    Profile* otr_profile)
    : otr_profile_(otr_profile),
      original_template_url_service_(
          TemplateURLServiceFactory::GetForProfile(
              otr_profile_->GetOriginalProfile())),
      otr_template_url_service_(
          TemplateURLServiceFactory::GetForProfile(otr_profile_)) {
  use_alternative_search_engine_provider_.Init(
      kUseAlternativeSearchEngineProvider,
      otr_profile_->GetOriginalProfile()->GetPrefs(),
      base::Bind(&SearchEngineProviderControllerBase::OnPreferenceChanged,
                 base::Unretained(this)));

  // This class should be destroyed together with otr profile's template url
  // service. If not, this can access dangling otr profile would be accessed.
  destroyer_ = new Destroyer(this, otr_template_url_service_);

  auto data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      otr_profile->GetPrefs(),
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO);
  alternative_search_engine_url_.reset(new TemplateURL(*data));
}

SearchEngineProviderControllerBase::~SearchEngineProviderControllerBase() {
}

void SearchEngineProviderControllerBase::OnPreferenceChanged(
    const std::string& pref_name) {
  DCHECK(pref_name == kUseAlternativeSearchEngineProvider);

  ConfigureSearchEngineProvider();
}

bool
SearchEngineProviderControllerBase::UseAlternativeSearchEngineProvider() const {
  return use_alternative_search_engine_provider_.GetValue();
}

void SearchEngineProviderControllerBase::
ChangeToAlternativeSearchEngineProvider() {
  otr_template_url_service_->SetUserSelectedDefaultSearchProvider(
      alternative_search_engine_url_.get());
}

void SearchEngineProviderControllerBase::
ChangeToNormalWindowSearchEngineProvider() {
  TemplateURL normal_url(
      original_template_url_service_->GetDefaultSearchProvider()->data());
  otr_template_url_service_->SetUserSelectedDefaultSearchProvider(
      &normal_url);
}

