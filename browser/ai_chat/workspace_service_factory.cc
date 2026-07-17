// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/workspace_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/components/ai_chat/core/browser/workspace/workspace_service.h"
#include "chrome/browser/profiles/profile_selections.h"

namespace ai_chat {

// static
WorkspaceServiceFactory* WorkspaceServiceFactory::GetInstance() {
  static base::NoDestructor<WorkspaceServiceFactory> instance;
  return instance.get();
}

// static
WorkspaceService* WorkspaceServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<WorkspaceService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

WorkspaceServiceFactory::WorkspaceServiceFactory()
    : ProfileKeyedServiceFactory(
          "WorkspaceService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) {}

WorkspaceServiceFactory::~WorkspaceServiceFactory() = default;

std::unique_ptr<KeyedService>
WorkspaceServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<WorkspaceService>();
}

}  // namespace ai_chat
