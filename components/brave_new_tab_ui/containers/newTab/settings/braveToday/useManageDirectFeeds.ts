// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import getBraveNewsAPI, { Publisher, Publishers, PublisherType } from '../../../../api/brave_news'
import * as todayActions from '../../../../actions/today_actions'

export enum FeedInputValidity {
  Valid,
  NotValid,
  Pending
}

const regexThe = /^the /

function comparePublishersByName (a: Publisher, b: Publisher) {
  // TODO(petemill): culture-independent 'the' removal, perhaps
  // do the sorting on the service-side.
  const aName = a.publisherName.toLowerCase().replace(regexThe, '')
  const bName = b.publisherName.toLowerCase().replace(regexThe, '')
  if (aName < bName) {
    return -1
  }
  if (aName > bName) {
    return 1
  }
  return 0
}

export default function useManageDirectFeeds (publishers?: Publishers) {
  const dispatch = useDispatch()
  // Memoize user feeds
  const userFeeds = React.useMemo<Publisher[] | undefined>(() => {
    if (!publishers) {
      return
    }
    return Object.values(publishers)
      .filter(p => p.type === PublisherType.DIRECT_SOURCE)
      .sort(comparePublishersByName)
  }, [publishers])
  // Function to turn direct feed off
  const onRemoveDirectFeed = function (directFeed: Publisher) {
    dispatch(todayActions.removeDirectFeed({ directFeed }))
  }
  const [feedInputText, setFeedInputText] = React.useState<string>()
  const [feedInputIsValid, setFeedInputIsValid] = React.useState<FeedInputValidity>(FeedInputValidity.Valid)
  const onChangeFeedInput = (e: React.ChangeEvent<HTMLInputElement>) => {
    setFeedInputText(e.target.value)
    setFeedInputIsValid(FeedInputValidity.Valid)
  }
  const onAddSource = React.useCallback(async () => {
    if (!feedInputText) {
      return
    }
    setFeedInputIsValid(FeedInputValidity.Pending)
    const api = getBraveNewsAPI()
    const result = await api.subscribeToNewDirectFeed({ url: feedInputText })
    if (!result.isValidFeed) {
      setFeedInputIsValid(FeedInputValidity.NotValid)
      return
    }
    if (result.isDuplicate) {
      setFeedInputIsValid(FeedInputValidity.NotValid)
      return
    }
    setFeedInputIsValid(FeedInputValidity.Valid)
    setFeedInputText('')
    dispatch(todayActions.dataReceived({ publishers: result.publishers }))
  }, [feedInputText, feedInputIsValid, setFeedInputIsValid, dispatch])

  return {
    userFeeds,
    feedInputIsValid,
    feedInputText,
    onRemoveDirectFeed,
    onChangeFeedInput,
    onAddSource
  }
}
