/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#include <memory>
#include "brave/components/private_channel/browser/private_channel_service_factory.h"
//#include "brave/components/private_channel/browser/private_channel_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
PrivateChannelServiceFactory* PrivateChannelServiceFactory::GetInstance() {
  return base::Singleton<PrivateChannelServiceFactory>::get();
}

PrivateChannelServiceFactory::PrivateChannelServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "PrivateChannelService",
          BrowserContextDependencyManager::GetInstance()) {
}

PrivateChannelServiceFactory::~PrivateChannelServiceFactory() {
}
