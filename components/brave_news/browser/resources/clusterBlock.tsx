// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { FeedItemMetadata } from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import Card from './card'

export default function ClusterBlock(props: {
  channelOrTopic: string
  articles: FeedItemMetadata[]
}) {
  return <Card>CLUSTER BLOCK FOR {props.channelOrTopic} PLACEHOLDER</Card>
}
