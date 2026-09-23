/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspaces/workspace_service_factory.h"

#include <memory>

#include "brave/browser/workspaces/features.h"
#include "brave/browser/workspaces/pref_names.h"
#include "brave/browser/workspaces/workspace_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "components/pref_registry/pref_registry_syncable.h"

// static
WorkspaceServiceFactory* WorkspaceServiceFactory::GetInstance() {
  static base::NoDestructor<WorkspaceServiceFactory> instance;
  return instance.get();
}

// static
WorkspaceService* WorkspaceServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<WorkspaceService*>(
      GetInstance()->GetServiceForBrowserContext(profile, /*create=*/true));
}

WorkspaceServiceFactory::WorkspaceServiceFactory()
    : ProfileKeyedServiceFactory("WorkspaceService",
                                 ProfileSelections::BuildForRegularProfile()) {}

WorkspaceServiceFactory::~WorkspaceServiceFactory() = default;

std::unique_ptr<KeyedService>
WorkspaceServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(features::kWorkspaces)) {
    return nullptr;
  }

  Profile* profile = Profile::FromBrowserContext(context);
  return std::make_unique<WorkspaceService>(*profile);
}

void WorkspaceServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(kWorkspacesMetadataPref);
}

bool WorkspaceServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}
