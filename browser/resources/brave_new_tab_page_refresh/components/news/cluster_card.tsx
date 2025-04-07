/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Cluster, ClusterType, FeedItemMetadata } from '../../api/news'
import { CategoryIcon } from './category_icon'
import { CategoryName } from './category_name'
import { ArticleCard } from './article_card'

import { style } from './cluster_card.style'

interface Props {
  clusterItem: Cluster
  articleIndex: number
}

export function ClusterCard(props: Props) {
  const { clusterItem } = props
  const { id } = clusterItem

  function renderItemContent(data: FeedItemMetadata | null, index: number) {
    if (!data) {
      return null
    }
    return (
      <ArticleCard
        item={data}
        articleIndex={index}
        hideCategory={clusterItem.type === ClusterType.CHANNEL}
      />
    )
  }

  return (
    <div data-css-scope={style.scope}>
      <h3>
        <CategoryIcon category={id} />
        <CategoryName category={id} />
      </h3>
      {
        clusterItem.articles.map((item, i) => (
          <div key={i} className='item'>
            {
              renderItemContent(
                item.article?.data ?? item.hero?.data ?? null,
                props.articleIndex + i)
            }
          </div>
        ))
      }
    </div>
  )
}
