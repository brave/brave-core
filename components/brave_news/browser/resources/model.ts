// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  FeedItemMetadata,
  Publisher
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'

export type ArticleElements =
  | {
      type: 'hero'
      article: FeedItemMetadata
    }
  | {
      type: 'inline'
      article: FeedItemMetadata
      isDiscover: boolean
    }

export type Elements =
  | ArticleElements
  | {
      type: 'advert'
    }
  | {
      type: 'discover'
      publishers: Publisher[]
    }
  | {
      type: 'cluster'
      clusterType:
                   | {
            type: 'channel'
            id: string
          }
        | {
            type: 'topic'
            id: string
          }
      elements: ArticleElements[]
    }
