/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { useLocale } from '../context/locale_context'
import { useAppState, useAppActions } from '../context/app_model_context'
import { isNewsPublisherEnabled } from '../../models/news'
import { inlineCSSVars } from '../../lib/inline_css_vars'
import { getKeyedColor } from '../../lib/keyed_colors'
import { SafeImage } from '../common/safe_image'
import classNames from '$web-common/classnames'

import { style } from './discover_card.style'

interface Props {
  publisherIds: string[]
}

export function DiscoverCard(props: Props) {
  const { getString } = useLocale()
  const actions = useAppActions()
  const publishers = useAppState((s) => s.newsPublishers)

  function renderPublisher(id: string) {
    const publisher = publishers[id]
    if (!publisher) {
      return null
    }

    const enabled = isNewsPublisherEnabled(publisher)

    const background =
      publisher.backgroundColor ||
      getKeyedColor(publisher.feedSourceUrl || id)

    return (
      <div key={id} style={inlineCSSVars({ '--cover-background': background })}>
        <div className={classNames({'cover': true, 'enabled': enabled })}>
          {publisher.coverUrl && <SafeImage src={publisher.coverUrl} />}
          <Button
            size='tiny'
            fab
            onClick={() => actions.setNewsPublisherEnabled(id, !enabled)}
          >
            <Icon name={enabled ? 'minus' : 'plus-add'} />
          </Button>
        </div>
        {publisher.publisherName}
      </div>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      <h3>
        <Icon name='star-outline' />
        {getString('newsDiscoverTitle')}
      </h3>
      <div className='publishers'>
        {props.publisherIds.map(renderPublisher)}
      </div>
    </div>
  )
}
