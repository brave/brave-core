/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { getString } from '../../lib/strings'
import { FeedV2Error } from '../../state/news_state'
import { useNewsActions } from '../../context/news_context'

import { style } from './feed_error_card.style'

interface Props {
  error: FeedV2Error
}

export function FeedErrorCard(props: Props) {
  return (
    <div data-css-scope={style.scope}>
      <FeedErrorView error={props.error} />
    </div>
  )
}

function FeedErrorView(props: { error: FeedV2Error }) {
  switch (props.error) {
    case FeedV2Error.ConnectionError: return <ConnectionError />
    case FeedV2Error.NoArticles: return <NoArticles />
    case FeedV2Error.NoFeeds: return <NoFeeds />
  }
  console.error('Unhandled feed error', props.error)
  return null
}

function ConnectionError() {
  const actions = useNewsActions()
  return <>
    <div className='graphic' />
    <h3>{getString('newsConnectionErrorTitle')}</h3>
    <p>{getString('newsConnectionErrorText')}</p>
    <Button onClick={() => actions.updateFeed()}>
      {getString('newsRefreshButtonLabel')}
    </Button>
  </>
}

function NoArticles() {
  return <>
    <div className='graphic no-articles' />
    <h3>{getString('newsNoArticlesTitle')}</h3>
    <p>{getString('newsNoArticlesText')}</p>
  </>
}

function NoFeeds() {
  return <>
    <div className='graphic' />
    <h3>{getString('newsNoFeedsTitle')}</h3>
    <p>{getString('newsNoFeedsText')}</p>
    <Button>
      {getString('newsAddSourcesButtonLabel')}
    </Button>
  </>
}
