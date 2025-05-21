/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Cluster, ClusterType } from '../../state/news_state'
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

  return (
    <div data-css-scope={style.scope}>
      <h3>
        <CategoryIcon category={id} />
        <CategoryName category={id} />
      </h3>
      {clusterItem.articles.map((item, i) => {
        const data = item.article?.data ?? item.hero?.data ?? null
        if (!data) {
          return null
        }
        return (
          <div
            key={i}
            className='item'
          >
            <ArticleCard
              item={data}
              articleIndex={props.articleIndex + i}
              hideCategory={clusterItem.type === ClusterType.CHANNEL}
            />
          </div>
        )
      })}
    </div>
  )
}
