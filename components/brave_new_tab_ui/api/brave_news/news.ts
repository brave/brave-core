// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { Publisher, PublisherType, UserEnabled } from 'gen/brave/components/brave_today/common/brave_news.mojom.m'
export const isPublisherEnabled = (publisher: Publisher) => {
  if (!publisher) return false

  // Direct Sources are enabled if they're available.
  if (publisher.type === PublisherType.DIRECT_SOURCE) return true

  // Publishers enabled via channel are not shown in the sidebar.
  return publisher.userEnabledStatus === UserEnabled.ENABLED
}

export const isDirectFeed = (publisher: Publisher) => {
  if (!publisher) return false
  return publisher.type === PublisherType.DIRECT_SOURCE
}
