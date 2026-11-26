/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sponsored_content/sponsored_content_service.h"

#include "base/check.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/sponsored_content/sponsored_content_component_installer.h"
#include "brave/components/sponsored_content/sponsored_content_component_public_keys.h"
#include "brave/components/sponsored_content/sponsored_content_country_code.h"
#include "brave/components/sponsored_content/sponsored_content_features.h"
#include "brave/components/sponsored_content/sponsored_content_on_demand_update.h"
#include "components/component_updater/component_updater_service.h"
#include "components/crx_file/id_util.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace sponsored_content {

namespace {

constexpr char kNewTabTakeoverJsonFilename[] = "campaigns.json";

constexpr char kSiteJsonFilename[] = "tiles.json";

constexpr char kConversionConfigJsonFilename[] = "conversion-config.json";

}  // namespace

SponsoredContentService::SponsoredContentService(
    variations::VariationsService* variations_service,
    component_updater::ComponentUpdateService* component_update_service)
    : variations_service_(variations_service),
      component_update_service_(component_update_service) {}

SponsoredContentService::~SponsoredContentService() = default;

void SponsoredContentService::StartTearDown() {
  component_update_service_ = nullptr;
  variations_service_ = nullptr;
}

void SponsoredContentService::AddObserver(
    SponsoredContentServiceObserver* const observer) {
  CHECK(observer);

  if (observers_.HasObserver(observer)) {
    // Already observing with sponsored content already loaded. Replay it,
    // rather than making this caller wait for the next update check.
    if (installed_dir_) {
      LoadAndNotifyContent(*installed_dir_);
    }
    return;
  }

  const bool was_empty = observers_.empty();

  observers_.AddObserver(observer);

  if (was_empty) {
    RegisterSponsoredContentComponent();
  } else if (installed_dir_) {
    // Component already registered and loaded by another observer. Deliver
    // the current sponsored content to the newly added observer too.
    LoadAndNotifyContent(*installed_dir_);
  }
}

void SponsoredContentService::AddObserverForOverridePath(
    SponsoredContentServiceObserver* const observer) {
  CHECK(observer);

  if (!observers_.HasObserver(observer)) {
    observers_.AddObserver(observer);
  }
}

void SponsoredContentService::RemoveObserver(
    SponsoredContentServiceObserver* const observer) {
  CHECK(observer);

  observers_.RemoveObserver(observer);

  if (observers_.empty()) {
    UnregisterSponsoredContentComponent();
  }
}

const std::optional<std::string>& SponsoredContentService::GetComponentId()
    const {
  return component_id_;
}

void SponsoredContentService::RegisterSponsoredContentComponent() {
  const std::string variations_country_code = GetCountryCode();
  const std::optional<std::array<uint8_t, crypto::hash::kSha256Size>>
      public_key_sha256 = GetComponentPublicKeySHA256(variations_country_code);
  if (!public_key_sha256) {
    // No component is available for this country.
    return;
  }

  const std::string component_id =
      crx_file::id_util::GenerateIdFromHash(*public_key_sha256);

  if (component_id_ == component_id) {
    // Component already loaded. Replay the callback so observers added after
    // the initial load still receive the sponsored content.
    if (installed_dir_) {
      LoadAndNotifyContent(*installed_dir_);
    }
    return;
  }

  if (component_id_) {
    UnregisterSponsoredContentComponent();
  }
  component_id_ = component_id;

  VLOG(0) << "Registering Sponsored Content component for country "
          << variations_country_code << " with ID " << *component_id_;
  RegisterSponsoredContentComponentInstaller(
      component_update_service_, *public_key_sha256,
      absl::StrFormat("Sponsored Content (%s)", variations_country_code),
      base::BindRepeating(&SponsoredContentService::LoadAndNotifyContent,
                          weak_factory_.GetWeakPtr()));

  // Sponsored content goes stale faster than the browser's default 5-hour
  // component update interval, so check more frequently than that.
  update_check_callback_ =
      base::BindRepeating(&SponsoredContentService::CheckComponentUpdate,
                          weak_factory_.GetWeakPtr(), *component_id_);

  last_updated_at_ = base::Time::Now();

  ScheduleNextComponentUpdateCheck();
}

void SponsoredContentService::UnregisterSponsoredContentComponent() {
  if (!component_id_) {
    return;
  }

  VLOG(0) << "Unregistering Sponsored Content component with ID "
          << *component_id_;
  observers_.Notify(&SponsoredContentServiceObserver::
                        OnSponsoredContentComponentDidUnregister);

  // `component_update_service_` is cleared by `StartTearDown` before the
  // profile manager destroys per-profile observers, so it can be null here.
  if (component_update_service_) {
    component_update_service_->UnregisterComponent(*component_id_);
  }
  component_id_.reset();

  // Drop any in-progress callbacks bound to the now-unregistered component.
  weak_factory_.InvalidateWeakPtrs();
  installed_dir_.reset();

  update_check_callback_.Reset();
  update_check_timer_.Stop();
}

void SponsoredContentService::MaybeCheckForComponentUpdate() {
  if (ShouldCheckForComponentUpdate() && update_check_callback_) {
    update_check_callback_.Run();
  }
}

void SponsoredContentService::ForceComponentUpdate() {
  if (!component_id_) {
    return;
  }

  QueueSponsoredContentComponentUpdate(*component_id_);

  // Replays the current content immediately, rather than leaving the caller
  // with nothing until a genuinely newer version exists.
  if (installed_dir_) {
    LoadAndNotifyContent(*installed_dir_);
  }
}

void SponsoredContentService::LoadComponentFromOverridePath(
    const base::FilePath& installed_dir) {
  LoadAndNotifyContent(installed_dir);
}

void SponsoredContentService::LoadAndNotifyContent(
    const base::FilePath& installed_dir) {
  installed_dir_ = installed_dir;

  VLOG(6) << "Loading Sponsored Content from " << installed_dir;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&SponsoredContentService::MaybeReadJsonFile, installed_dir,
                     kNewTabTakeoverJsonFilename),
      base::BindOnce(&SponsoredContentService::
                         HandleSponsoredNewTabTakeoverContentCallback,
                     weak_factory_.GetWeakPtr(), installed_dir));

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&SponsoredContentService::MaybeReadJsonFile, installed_dir,
                     kSiteJsonFilename),
      base::BindOnce(
          &SponsoredContentService::HandleSponsoredSiteContentCallback,
          weak_factory_.GetWeakPtr(), installed_dir));

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&SponsoredContentService::MaybeReadJsonFile, installed_dir,
                     kConversionConfigJsonFilename),
      base::BindOnce(&SponsoredContentService::
                         HandleSponsoredConversionConfigContentCallback,
                     weak_factory_.GetWeakPtr(), installed_dir));
}

void SponsoredContentService::HandleSponsoredNewTabTakeoverContentCallback(
    const base::FilePath& installed_dir,
    std::optional<base::DictValue> dict) {
  observers_.Notify(&SponsoredContentServiceObserver::
                        OnSponsoredNewTabTakeoverContentDidUpdate,
                    installed_dir, dict);
}

void SponsoredContentService::HandleSponsoredSiteContentCallback(
    const base::FilePath& installed_dir,
    std::optional<base::DictValue> dict) {
  observers_.Notify(
      &SponsoredContentServiceObserver::OnSponsoredSiteContentDidUpdate,
      installed_dir, dict);
}

void SponsoredContentService::HandleSponsoredConversionConfigContentCallback(
    const base::FilePath& installed_dir,
    std::optional<base::DictValue> dict) {
  observers_.Notify(&SponsoredContentServiceObserver::
                        OnSponsoredConversionConfigContentDidUpdate,
                    installed_dir, dict);
}

///////////////////////////////////////////////////////////////////////////////

// static
std::optional<base::DictValue> SponsoredContentService::MaybeReadJsonFile(
    const base::FilePath& installed_dir,
    const std::string& json_filename) {
  const base::FilePath file_path = installed_dir.AppendASCII(json_filename);

  std::string contents;
  const bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    VLOG(6) << "Failed to read Sponsored Content JSON file " << json_filename;
    return std::nullopt;
  }

  return base::JSONReader::ReadDict(contents,
                                    base::JSON_PARSE_CHROMIUM_EXTENSIONS);
}

std::string SponsoredContentService::GetCountryCode() const {
  return GetVariationsCountryCode(variations_service_);
}

bool SponsoredContentService::ShouldCheckForComponentUpdate() const {
  // The component has not loaded yet.
  if (!last_updated_at_) {
    return false;
  }

  // The periodic check was missed, so this one is overdue.
  return base::Time::Now() - *last_updated_at_ >
         features::kComponentUpdateCheckAfter.Get();
}

void SponsoredContentService::ScheduleNextComponentUpdateCheck() {
  if (!update_check_callback_) {
    return;
  }

  const base::Time next_update_check_at =
      base::Time::Now() + features::kComponentUpdateCheckAfter.Get();
  update_check_timer_.Start(FROM_HERE, next_update_check_at,
                            base::BindOnce(update_check_callback_));

  if (component_id_) {
    VLOG(6) << "Scheduled update check for Sponsored Content component with "
               "ID "
            << *component_id_ << " at "
            << base::TimeFormatFriendlyDateAndTime(next_update_check_at);
  }
}

void SponsoredContentService::CheckComponentUpdate(
    const std::string& component_id) {
  last_updated_at_ = base::Time::Now();

  QueueSponsoredContentComponentUpdate(component_id);

  ScheduleNextComponentUpdateCheck();
}

}  // namespace sponsored_content
