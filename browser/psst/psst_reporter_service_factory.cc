// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_reporter_service_factory.h"

#include <memory>
#include <optional>

#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "brave/common/brave_channel_info.h"
#include "brave/components/psst/core/browser/psst_component_installer.h"
#include "brave/components/psst/core/browser/psst_report_uploader.h"
#include "brave/components/psst/core/browser/psst_reporter_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "components/component_updater/component_updater_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace {

std::optional<std::string> GetPsstComponentVersion(
    component_updater::ComponentUpdateService* component_update_service) {
  if (!component_update_service) {
    return std::nullopt;
  }

  auto components(component_update_service->GetComponents());
  auto it = std::find_if(
      components.begin(), components.end(),
      [](const auto& c) { return c.id == psst::kPsstComponentId; });

  if (it == components.end()) {
    return std::nullopt;
  }

  return it->version.GetString();
}

}  // namespace

// static
PsstReporterServiceFactory* PsstReporterServiceFactory::GetInstance() {
  static base::NoDestructor<PsstReporterServiceFactory> instance;
  return instance.get();
}

// static
psst::PsstReporterService* PsstReporterServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<psst::PsstReporterService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

PsstReporterServiceFactory::PsstReporterServiceFactory()
    : ProfileKeyedServiceFactory(
          "PsstReporterService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) {}

PsstReporterServiceFactory::~PsstReporterServiceFactory() = default;

std::unique_ptr<KeyedService>
PsstReporterServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto report_uploader = std::make_unique<psst::PsstErrorReportUploader>(
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess());
  auto cv_callback = base::BindRepeating(
      &GetPsstComponentVersion, g_browser_process->component_updater());
  auto cn_callback = base::BindRepeating(&brave::GetChannelName);

  return std::make_unique<psst::PsstReporterService>(
      std::move(cn_callback), std::move(cv_callback),
      std::move(report_uploader));
}
