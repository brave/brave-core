/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"

#include <algorithm>
#include <memory>

#include "base/check_op.h"
#include "base/command_line.h"
#include "base/debug/crash_logging.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/task/thread_pool.h"
#include "base/types/optional_ref.h"
#include "base/values.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_component_installer.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/sponsored_content/new_tab_takeover/ntp_sponsored_content_data.h"
#include "brave/components/ntp_background_images/browser/sponsored_content/site/ntp_sponsored_sites_data.h"
#include "brave/components/ntp_background_images/browser/switches.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/sponsored_content/sponsored_content_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/variations/pref_names.h"
#include "content/public/common/url_constants.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ntp_background_images {

namespace {

constexpr char kNTPManifestFile[] = "photo.json";

constexpr char kNewTabPageCachedSuperReferralComponentInfo[] =
    "brave.new_tab_page.cached_super_referral_component_info";
constexpr char kNewTabPageCachedSuperReferralComponentData[] =
    "brave.new_tab_page.cached_super_referral_component_data";
constexpr char kNewTabPageGetInitialSuperReferralComponentInProgress[] =
    "brave.new_tab_page.get_initial_sr_component_in_progress";
constexpr char kNewTabPageCachedSuperReferralCode[] =
    "brave.new_tab_page.cached_referral_code";

// If registered component is for sponsored content, it has
// `kNTPSponsoredManifestFile` in |installed_dir|.
std::string HandleComponentData(const base::FilePath& installed_dir,
                                const std::string& manifest_file) {
  const base::FilePath file_path = installed_dir.AppendASCII(manifest_file);

  std::string contents;
  const bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    SCOPED_CRASH_KEY_BOOL("Issue50267", "success", success);
    SCOPED_CRASH_KEY_BOOL("Issue50267", "empty_contents", contents.empty());
    SCOPED_CRASH_KEY_BOOL("Issue50267", "path_exists",
                          base::PathExists(file_path));
    SCOPED_CRASH_KEY_STRING64("Issue50267", "filename",
                              file_path.BaseName().AsUTF8Unsafe());
    SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason", "Invalid JSON");
    DUMP_WILL_BE_NOTREACHED();
    VLOG(6) << "Cannot read NTP component " << manifest_file
            << " manifest file";
  }

  return contents;
}

}  // namespace

// static
void NTPBackgroundImagesService::RegisterLocalStatePrefsForMigration(
    PrefRegistrySimple* registry) {
  // Added 10/2025
  registry->RegisterDictionaryPref(kNewTabPageCachedSuperReferralComponentInfo);
  registry->RegisterStringPref(kNewTabPageCachedSuperReferralComponentData,
                               std::string());
  registry->RegisterStringPref(kNewTabPageCachedSuperReferralCode,
                               std::string());
  registry->RegisterBooleanPref(
      kNewTabPageGetInitialSuperReferralComponentInProgress, false);
}

// static
void NTPBackgroundImagesService::MigrateObsoleteLocalStatePrefs(
    PrefService* local_state) {
  // Added 10/2025
  local_state->ClearPref(kNewTabPageCachedSuperReferralComponentInfo);
  local_state->ClearPref(kNewTabPageCachedSuperReferralComponentData);
  local_state->ClearPref(kNewTabPageCachedSuperReferralCode);
  local_state->ClearPref(kNewTabPageGetInitialSuperReferralComponentInProgress);
}

NTPBackgroundImagesService::NTPBackgroundImagesService(
    component_updater::ComponentUpdateService* component_update_service,
    sponsored_content::SponsoredContentService& sponsored_content_service,
    PrefService* pref_service)
    : component_update_service_(component_update_service),
      sponsored_content_service_(sponsored_content_service),
      pref_service_(pref_service) {}

NTPBackgroundImagesService::~NTPBackgroundImagesService() = default;

void NTPBackgroundImagesService::Init() {
  pref_change_registrar_.Init(pref_service_);

  // Flag override for testing or demo purposes
  base::FilePath override_sponsored_images_component_path(
      base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(
          switches::kOverrideSponsoredImagesComponentPath));
  if (!override_sponsored_images_component_path.empty()) {
    DVLOG(6)
        << "NTP Sponsored Images test data will be loaded from local path at: "
        << override_sponsored_images_component_path.LossyDisplayName();
    // Observes without registering the real, country-based component, so it
    // cannot later overwrite this override content once installed.
    sponsored_content_service_->AddObserverForOverridePath(this);
    // `Reset` is a no-op if not yet observing, so this stays safe whether
    // `Init` is called for the first time or again, e.g. by a test calling
    // it explicitly after the browser process already called it once.
    sponsored_content_service_observation_.Reset();
    sponsored_content_service_observation_.Observe(
        base::to_address(sponsored_content_service_));
    sponsored_content_service_->LoadComponentFromOverridePath(
        override_sponsored_images_component_path);
  } else {
    RegisterBackgroundImagesComponent();

    pref_change_registrar_.Add(
        variations::prefs::kVariationsCountry,
        base::BindRepeating(
            &NTPBackgroundImagesService::OnVariationsCountryPrefChanged,
            weak_factory_.GetWeakPtr()));
  }
}

void NTPBackgroundImagesService::StartTearDown() {
  component_update_service_ = nullptr;
  sponsored_content_service_observation_.Reset();
  pref_service_ = nullptr;
  pref_change_registrar_.RemoveAll();
}

void NTPBackgroundImagesService::MaybeCheckForSponsoredComponentUpdate() {
  sponsored_content_service_->MaybeCheckForComponentUpdate();
}

void NTPBackgroundImagesService::ForceSponsoredComponentUpdate() {
  sponsored_content_service_->ForceComponentUpdate();
}

void NTPBackgroundImagesService::ResetSponsoredContentData() {
  sponsored_content_data_.reset();
  sponsored_content_data_excluding_dynamic_.reset();
  observers_.Notify(&Observer::OnSponsoredContentDidUpdate,
                    /*data=*/base::DictValue());
  observers_.Notify(&Observer::DeprecatedOnSponsoredContentDidUpdate,
                    /*data=*/nullptr);
}

void NTPBackgroundImagesService::RegisterBackgroundImagesComponent() {
  VLOG(6) << "Registering NTP Background Images component";
  RegisterNTPBackgroundImagesComponent(
      component_update_service_,
      base::BindRepeating(&NTPBackgroundImagesService::OnComponentReady,
                          weak_factory_.GetWeakPtr()));
}

void NTPBackgroundImagesService::RegisterSponsoredImagesComponent() {
  if (sponsored_content_service_observation_.IsObserving()) {
    // Already observing; `AddObserver` replays the current content for this
    // additional profile instead of re-adding an existing observer.
    sponsored_content_service_->AddObserver(this);
    return;
  }

  sponsored_content_service_observation_.Observe(
      base::to_address(sponsored_content_service_));
}

void NTPBackgroundImagesService::UnregisterSponsoredImagesComponent() {
  sponsored_content_service_observation_.Reset();

  // Drops any in-flight filter/parse reply so it cannot resurrect stale data
  // after this clears it.
  sponsored_content_weak_factory_.InvalidateWeakPtrs();

  sponsored_content_installed_dir_.reset();

  ResetSponsoredContentData();

  sponsored_sites_data_.reset();
  observers_.Notify(&Observer::OnSponsoredSitesDataDidUpdate);
}

void NTPBackgroundImagesService::AddSponsoredImagesOptedInProfile() {
  ++sponsored_images_opted_in_profile_count_;

  // Always register, even if already registered by another profile.
  // `RegisterSponsoredImagesComponent()` replays the ready callback for an
  // already-loaded component so this profile still receives the data.
  RegisterSponsoredImagesComponent();
}

void NTPBackgroundImagesService::RemoveSponsoredImagesOptedInProfile() {
  CHECK_GT(sponsored_images_opted_in_profile_count_, 0U);
  if (--sponsored_images_opted_in_profile_count_ == 0U) {
    UnregisterSponsoredImagesComponent();
  }
}

void NTPBackgroundImagesService::OnVariationsCountryPrefChanged() {
  // Defers first registration until ads init completes, to prevent a race,
  // and skips re-registering when no profile would use the content anyway.
  if (sponsored_content_service_observation_.IsObserving() &&
      sponsored_images_opted_in_profile_count_ > 0) {
    sponsored_content_service_->RegisterSponsoredContentComponent();
  }
}

void NTPBackgroundImagesService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void NTPBackgroundImagesService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool NTPBackgroundImagesService::HasObserver(Observer* observer) {
  return observers_.HasObserver(observer);
}

NTPBackgroundImagesData* NTPBackgroundImagesService::GetBackgroundImagesData()
    const {
  if (background_images_data_ && background_images_data_->IsValid()) {
    return background_images_data_.get();
  }

  return nullptr;
}

NTPSponsoredContentData* NTPBackgroundImagesService::GetNewTabTakeover(
    bool supports_dynamic_new_tab_takeover) const {
  NTPSponsoredContentData* const sponsored_content_data =
      supports_dynamic_new_tab_takeover
          ? sponsored_content_data_.get()
          : sponsored_content_data_excluding_dynamic_.get();
  if (!sponsored_content_data || !sponsored_content_data->IsValid()) {
    return nullptr;
  }

  return sponsored_content_data;
}

NTPSponsoredSitesData* NTPBackgroundImagesService::GetSponsoredSitesData()
    const {
  if (!sponsored_sites_data_ || !sponsored_sites_data_->IsValid()) {
    return nullptr;
  }

  return sponsored_sites_data_.get();
}

std::optional<base::FilePath>
NTPBackgroundImagesService::MaybeGetSponsoredSiteImageFilePath(
    const base::FilePath& request_path) const {
  if (!sponsored_content_installed_dir_) {
    return std::nullopt;
  }

  const NTPSponsoredSitesData* const sites_data = GetSponsoredSitesData();
  if (!sites_data) {
    return std::nullopt;
  }

  // `request_path` is derived from a URL path, which always uses "/" as a
  // separator; normalize it to the platform separator so it can be compared
  // against and appended to `base::FilePath`s built from installed_dir.
  const base::FilePath normalized_request_path =
      request_path.NormalizePathSeparators();

  // Reject any path that could escape the component directory.
  if (normalized_request_path.ReferencesParent() ||
      normalized_request_path.IsAbsolute()) {
    return std::nullopt;
  }

  // Only serve images that are actually referenced by a currently active
  // sponsored site; reject requests for any other file that happens to sit
  // in the component's installed directory (e.g. the manifest files). Compare
  // full relative paths, not just filenames, since a site's image may live in
  // a subdirectory of the component (e.g. a per-campaign folder).
  const bool is_known_site_image = std::ranges::any_of(
      sites_data->sites,
      [&normalized_request_path](const NTPSponsoredSite& site) {
        const GURL image_url(site.relative_image_url_spec);
        std::string_view path = image_url.path();
        if (path.size() <= 1) {
          return false;
        }
        return base::FilePath::FromUTF8Unsafe(path.substr(1))
                   .NormalizePathSeparators() == normalized_request_path;
      });
  if (!is_known_site_image) {
    return std::nullopt;
  }

  return sponsored_content_installed_dir_->Append(normalized_request_path);
}

const std::optional<std::string>&
NTPBackgroundImagesService::GetSponsoredImagesComponentId() const {
  return sponsored_content_service_->GetComponentId();
}

void NTPBackgroundImagesService::OnComponentReady(
    const base::FilePath& installed_dir) {
  background_images_installed_dir_ = installed_dir;

  VLOG(6) << "NTP Background Images component is ready";

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&HandleComponentData, installed_dir, kNTPManifestFile),
      base::BindOnce(&NTPBackgroundImagesService::OnGetComponentJsonData,
                     weak_factory_.GetWeakPtr()));
}

void NTPBackgroundImagesService::OnGetComponentJsonData(
    const std::string& json_string) {
  background_images_data_ = std::make_unique<NTPBackgroundImagesData>(
      json_string, background_images_installed_dir_);

  for (auto& observer : observers_) {
    observer.OnBackgroundImagesDataDidUpdate(background_images_data_.get());
  }
}

void NTPBackgroundImagesService::OnSponsoredNewTabTakeoverContentDidUpdate(
    const base::FilePath& installed_dir,
    base::optional_ref<const base::DictValue> dict) {
  sponsored_content_installed_dir_ = installed_dir;

  if (!dict) {
    // Drops any in-flight filter reply before resetting data, so it cannot
    // resurrect stale content afterward.
    sponsored_content_weak_factory_.InvalidateWeakPtrs();
    ResetSponsoredContentData();
    return;
  }

  // `dict` is owned by `SponsoredContentService`, so `Clone` it to hand a
  // copy to the background task.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&NTPBackgroundImagesService::FilterNewTabTakeoverCampaigns,
                     dict->Clone(), installed_dir),
      base::BindOnce(
          &NTPBackgroundImagesService::FilterNewTabTakeoverCampaignsCallback,
          sponsored_content_weak_factory_.GetWeakPtr(), installed_dir));
}

void NTPBackgroundImagesService::OnSponsoredSiteContentDidUpdate(
    const base::FilePath& installed_dir,
    base::optional_ref<const base::DictValue> dict) {
  sponsored_content_installed_dir_ = installed_dir;

  if (!dict) {
    // Drops any in-flight parse reply before resetting data, so it cannot
    // resurrect stale content afterward.
    sponsored_content_weak_factory_.InvalidateWeakPtrs();
    sponsored_sites_data_.reset();
    observers_.Notify(&Observer::OnSponsoredSitesDataDidUpdate);
    return;
  }

  const std::string url_prefix =
      absl::StrFormat("%s%s%s/", content::kChromeUIScheme,
                      url::kStandardSchemeSeparator, kSponsoredSiteImageHost);

  // `dict` is owned by `SponsoredContentService`, so `Clone` it to hand a
  // copy to the background task.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(
          &NTPBackgroundImagesService::MaybeParseSponsoredSiteContent,
          dict->Clone(), installed_dir, url_prefix),
      base::BindOnce(
          &NTPBackgroundImagesService::ParseSponsoredSiteContentCallback,
          sponsored_content_weak_factory_.GetWeakPtr()));
}

void NTPBackgroundImagesService::OnSponsoredConversionConfigContentDidUpdate(
    const base::FilePath& installed_dir,
    base::optional_ref<const base::DictValue> dict) {
  // `AdsServiceImpl` observes `SponsoredContentService` directly for this
  // content, so there is nothing further to forward here.
  sponsored_content_installed_dir_ = installed_dir;
}

void NTPBackgroundImagesService::OnSponsoredContentComponentDidUnregister() {
  // Drops any in-flight filter/parse reply before resetting data, so it
  // cannot resurrect stale content afterward.
  sponsored_content_weak_factory_.InvalidateWeakPtrs();

  // Clears stale content immediately rather than waiting for a replacement
  // component (if any) to install and deliver fresh content.
  sponsored_content_installed_dir_.reset();
  ResetSponsoredContentData();
  sponsored_sites_data_.reset();
  observers_.Notify(&Observer::OnSponsoredSitesDataDidUpdate);
}

// static
base::DictValue NTPBackgroundImagesService::FilterNewTabTakeoverCampaigns(
    base::DictValue dict,
    const base::FilePath& installed_dir) {
  FilterCampaigns(dict, installed_dir);
  return dict;
}

void NTPBackgroundImagesService::FilterNewTabTakeoverCampaignsCallback(
    const base::FilePath& installed_dir,
    base::DictValue dict) {
  sponsored_content_data_ =
      std::make_unique<NTPSponsoredContentData>(dict, installed_dir);

  sponsored_content_data_excluding_dynamic_ =
      std::make_unique<NTPSponsoredContentData>(*sponsored_content_data_);
  for (auto& campaign : sponsored_content_data_excluding_dynamic_->campaigns) {
    std::erase_if(campaign.creatives, [](const auto& creative) {
      return creative.wallpaper_type == WallpaperType::kDynamicNewTabTakeover;
    });
  }
  std::erase_if(
      sponsored_content_data_excluding_dynamic_->campaigns,
      [](const auto& campaign) { return campaign.creatives.empty(); });

  observers_.Notify(&Observer::OnSponsoredContentDidUpdate, dict);
  observers_.Notify(&Observer::DeprecatedOnSponsoredContentDidUpdate,
                    sponsored_content_data_.get());
}

// static
std::optional<NTPSponsoredSitesData>
NTPBackgroundImagesService::MaybeParseSponsoredSiteContent(
    base::DictValue dict,
    const base::FilePath& installed_dir,
    const std::string& url_prefix) {
  NTPSponsoredSitesData sites_data(dict, installed_dir, url_prefix);
  if (!sites_data.IsValid()) {
    return std::nullopt;
  }

  return sites_data;
}

void NTPBackgroundImagesService::ParseSponsoredSiteContentCallback(
    std::optional<NTPSponsoredSitesData> sites_data) {
  if (!sites_data) {
    sponsored_sites_data_.reset();
  } else {
    sponsored_sites_data_ =
        std::make_unique<NTPSponsoredSitesData>(std::move(*sites_data));
  }

  observers_.Notify(&Observer::OnSponsoredSitesDataDidUpdate);
}

}  // namespace ntp_background_images
