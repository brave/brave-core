/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { useLocale } from '../context/locale_context'
import { useAppActions } from '../context/app_model_context'
import { NewsItem, getNewsPublisherName } from '../../models/news'
import { CategoryIcon } from './category_icon'
import { CategoryName } from './category_name'
import { SafeImage } from '../common/safe_image'
import { Link } from '../common/link'
import { Popover } from '../common/popover'
import formatMessage from '$web-common/formatMessage'

import { style } from './article_card.style'

interface Props {
  item: NewsItem
  isHero?: boolean
  hideCategory?: boolean
}

export function ArticleCard(props: Props) {
  const { getString } = useLocale()
  const actions = useAppActions()
  const [showMenu, setShowMenu] = React.useState(false)
  const { item } = props

  return (
    <div data-css-scope={style.scope}>
      <Link url={item.url}>
        {
          props.isHero &&
            <div className='hero'>
              <SafeImage src={item.imageUrl} />
            </div>
        }
        <div className='metadata'>
          <span>{getNewsPublisherName(item)}</span>
          {
            !props.hideCategory && <>
              <span>•</span>
              <CategoryIcon category={item.categoryName} />
              <span><CategoryName category={item.categoryName} /></span>
            </>
          }
          <span>•</span>
          <span>{item.relativeTimeDescription}</span>
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
                  actions.setNewsPublisherEnabled(item.publisherId, false)
                  setShowMenu(false)
                }}
              >
                {
                  formatMessage(getString('newsHidePublisherLabel'), [
                    getNewsPublisherName(item)
                  ])
                }
              </button>
            </div>
          </Popover>
        </div>
        <div className='preview'>
          {item.title}
          {!props.isHero && <SafeImage src={item.imageUrl} />}
        </div>
      </Link>
    </div>
  )
}
