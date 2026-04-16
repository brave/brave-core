/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspace/brave_workspace_service_factory.h"

#include <memory>

#include "brave/browser/workspace/brave_workspace_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"

// static
BraveWorkspaceServiceFactory* BraveWorkspaceServiceFactory::GetInstance() {
  static base::NoDestructor<BraveWorkspaceServiceFactory> instance;
  return instance.get();
}

// static
BraveWorkspaceService* BraveWorkspaceServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<BraveWorkspaceService*>(
      GetInstance()->GetServiceForBrowserContext(profile, /*create=*/true));
}

BraveWorkspaceServiceFactory::BraveWorkspaceServiceFactory()
    : ProfileKeyedServiceFactory("BraveWorkspaceService",
                                 ProfileSelections::BuildForRegularProfile()) {}

BraveWorkspaceServiceFactory::~BraveWorkspaceServiceFactory() = default;

std::unique_ptr<KeyedService>
BraveWorkspaceServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<BraveWorkspaceService>(
      Profile::FromBrowserContext(context));
}

bool BraveWorkspaceServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return false;
}
