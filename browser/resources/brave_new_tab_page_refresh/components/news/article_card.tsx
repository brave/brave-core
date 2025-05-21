/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import {
  FeedItemMetadata,
  getNewsPublisherName,
  getNewsItemImage,
} from '../../state/news_state'

import { useNewsActions } from '../../context/news_context'
import { getString } from '../../lib/strings'
import { CategoryIcon } from './category_icon'
import { CategoryName } from './category_name'
import { SafeImage } from '../common/safe_image'
import { Link } from '../common/link'
import { Popover } from '../common/popover'
import { VisibilityTracker } from '../common/visibility_tracker'
import { formatString } from '$web-common/formatString'

import { style } from './article_card.style'

interface Props {
  item: FeedItemMetadata
  articleIndex: number
  isHero?: boolean
  hideCategory?: boolean
}

export function ArticleCard(props: Props) {
  const actions = useNewsActions()
  const [showMenu, setShowMenu] = React.useState(false)
  const { item } = props

  const imageURL = getNewsItemImage(item)

  return (
    <div
      className='news-article'
      data-css-scope={style.scope}
    >
      <Link
        url={item.url.url}
        openInNewTab
        onClick={() => actions.onCardVisited(props.articleIndex)}
      >
        {props.isHero && (
          <div className='hero'>
            <SafeImage
              src={imageURL}
              targetSize={{ width: 508, height: 269 }}
            />
          </div>
        )}
        <div className='metadata'>
          <span>{getNewsPublisherName(item)}</span>
          {!props.hideCategory && item.categoryName && (
            <>
              <span>•</span>
              <CategoryIcon category={item.categoryName} />
              <span>
                <CategoryName category={item.categoryName} />
              </span>
            </>
          )}
          {item.relativeTimeDescription && (
            <>
              <span>•</span>
              <span>{item.relativeTimeDescription}</span>
            </>
          )}
          <div className='actions'>
            <button
              className={showMenu ? 'article-menu-anchor' : ''}
              onClick={(event) => {
                event.preventDefault()
                setShowMenu(true)
              }}
            >
              <Icon name='more-horizontal' />
            </button>
          </div>
          <Popover
            isOpen={showMenu}
            className='article-menu'
            onClose={() => setShowMenu(false)}
          >
            <div className='popover-menu'>
              <button
                onClick={(event) => {
                  event.preventDefault()
                  actions.setPublisherEnabled(item.publisherId, false)
                  setShowMenu(false)
                }}
              >
                {formatString(getString(S.BRAVE_NEWS_HIDE_CONTENT_FROM), [
                  getNewsPublisherName(item),
                ])}
              </button>
            </div>
          </Popover>
        </div>
        <div className='preview'>
          {item.title}
          {!props.isHero && (
            <SafeImage
              src={imageURL}
              targetSize={{ width: 96, height: 64 }}
            />
          )}
        </div>
      </Link>
      <VisibilityTracker
        onVisible={() => actions.onNewCardsViewed(props.articleIndex)}
      />
    </div>
  )
}
