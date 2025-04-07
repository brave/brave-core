/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { useLocale } from '../context/locale_context'
import { useAppActions } from '../context/app_model_context'
import { NewsFeedError } from '../../models/news'

import { style } from './feed_error_card.style'

interface Props {
  error: NewsFeedError
}

export function FeedErrorCard(props: Props) {
  function renderError() {
    switch (props.error) {
      case 'connection-error': return <ConnectionError />
      case 'no-articles': return <NoArticles />
      case 'no-feeds': return <NoFeeds />
    }
  }
  return (
    <div data-css-scope={style.scope} className={props.error}>
      {renderError()}
    </div>
  )
}

function ConnectionError() {
  const { getString } = useLocale()
  const actions = useAppActions()
  return <>
    <div className='graphic' />
    <h3>{getString('newsConnectionErrorTitle')}</h3>
    <p>{getString('newsConnectionErrorText')}</p>
    <Button onClick={() => actions.updateNewsFeed()}>
      {getString('newsRefreshButtonLabel')}
    </Button>
  </>
}

function NoArticles() {
  const { getString } = useLocale()
  return <>
    <div className='graphic' />
    <h3>{getString('newsNoArticlesTitle')}</h3>
    <p>{getString('newsNoArticlesText')}</p>
  </>
}

function NoFeeds() {
  const { getString } = useLocale()
  return <>
    <div className='graphic' />
    <h3>{getString('newsNoFeedsTitle')}</h3>
    <p>{getString('newsNoFeedsText')}</p>
    <Button>
      {getString('newsAddSourcesButtonLabel')}
    </Button>
  </>
}
