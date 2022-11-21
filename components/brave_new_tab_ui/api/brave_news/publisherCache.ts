// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  BraveNewsControllerRemote,
  Publisher,
  PublisherListenerInterface,
  PublisherListenerReceiver,
  UserEnabled
} from "gen/brave/components/brave_today/common/brave_news.mojom.m";
import getBraveNewsController from ".";
import { CachingWrapper } from "./magicCache";
import { isDirectFeed } from "./news";

export class PublisherCachingWrapper
  extends CachingWrapper<Publisher>
  implements PublisherListenerInterface
{
  private receiver = new PublisherListenerReceiver(this);
  private controller: BraveNewsControllerRemote;

  constructor() {
    super();

    this.controller = getBraveNewsController();
    this.controller.addPublisherListener(
      this.receiver.$.bindNewPipeAndPassRemote()
    );
  }

  setPublisherFollowed(publisherId: string, enabled: boolean) {
    const copy = {
      ...this.cache
    };

    if (isDirectFeed(this.cache[publisherId]) && !enabled) {
      this.controller.setPublisherPref(publisherId, UserEnabled.ENABLED);
      delete copy[publisherId];
    } else {
      const status = enabled ? UserEnabled.ENABLED : UserEnabled.NOT_MODIFIED;
      this.controller.setPublisherPref(publisherId, status);
      copy[publisherId].userEnabledStatus = status;
    }

    this.change(copy);
  }
}
