/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { ClusterFeedItem, NewsFeedItem } from '../../models/news'
import { CategoryIcon } from './category_icon'
import { CategoryName } from './category_name'
import { ArticleCard } from './article_card'

import { style } from './cluster_card.style'

interface Props {
  clusterItem: ClusterFeedItem
}

export function ClusterCard(props: Props) {
  const { clusterItem } = props
  const { clusterId } = clusterItem

  function renderItemContent(item: NewsFeedItem) {
    switch (item.type) {
      case 'article':
      case 'hero':
        return (
          <ArticleCard
            item={item}
            hideCategory={clusterItem.clusterType === 'channel'}
          />
        )
      default:
        return null
    }
  }

  return (
    <div data-css-scope={style.scope}>
      <h3>
        <CategoryIcon category={clusterId} />
        <CategoryName category={clusterId} />
      </h3>
      {
        clusterItem.items.map((item, i) => (
          <div key={i} className='item'>
            {renderItemContent(item)}
          </div>
        ))
      }
    </div>
  )
}
