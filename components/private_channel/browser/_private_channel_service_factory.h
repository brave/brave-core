/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PRIVATE_CHANNEL_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_PRIVATE_CHANNEL_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class PrivateChannelService;
class Profile;

class PrivateChannelServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static PrivateChannelServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<PrivateChannelServiceFactory>;

  PrivateChannelServiceFactory();
  ~PrivateChannelServiceFactory() override;

  DISALLOW_COPY_AND_ASSIGN(PrivateChannelServiceFactory);
};

#endif  // BRAVE_BROWSER_PRIVATE_CHANNEL_SERVICE_FACTORY_H_
