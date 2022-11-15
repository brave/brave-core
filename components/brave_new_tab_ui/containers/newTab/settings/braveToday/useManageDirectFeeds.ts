// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import getBraveNewsAPI, { FeedSearchResultItem, Publisher, Publishers, PublisherType } from '../../../../api/brave_news'
import * as todayActions from '../../../../actions/today_actions'

export enum FeedInputValidity {
  Valid,
  NotValid,
  IsDuplicate,
  Pending,
  HasResults,
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

function isValidFeedUrl (feedInput: string): boolean {
  // is valid url?
  try {
    const url = new URL(feedInput)
    return ['http:', 'https:'].includes(url.protocol)
  } catch { }
  return false
}

type FeedSearchResultModel = FeedSearchResultItem & { status?: FeedInputValidity }

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
  const [feedSearchResults, setFeedSearchResults] = React.useState<FeedSearchResultModel[]>([])

  const onChangeFeedInput = (e: React.ChangeEvent<HTMLInputElement>) => {
    setFeedInputText(e.target.value)
    setFeedInputIsValid(FeedInputValidity.Valid)
  }

  const onSearchForSources = async () => {
    if (!feedInputText) {
      return
    }
    let feedUrlRaw = feedInputText
    // Default protocol that should catch most cases. Make sure
    // we check for validity after adding this prefix, and
    // not before.
    if (!feedUrlRaw.includes('://')) {
      feedUrlRaw = 'https://' + feedUrlRaw
    }
    if (!isValidFeedUrl(feedUrlRaw)) {
      setFeedInputIsValid(FeedInputValidity.NotValid)
      return
    }
    setFeedInputIsValid(FeedInputValidity.Pending)
    const api = getBraveNewsAPI()
    const { results } = await api.findFeeds({ url: feedUrlRaw })
    if (results.length === 0) {
      setFeedInputIsValid(FeedInputValidity.NotValid)
      return
    }
    if (results.length === 1) {
      const result = await api.subscribeToNewDirectFeed(results[0].feedUrl)
      if (!result.isValidFeed) {
        setFeedInputIsValid(FeedInputValidity.NotValid)
        return
      }
      if (result.isDuplicate) {
        setFeedInputIsValid(FeedInputValidity.IsDuplicate)
        return
      }
      // Valid
      setFeedInputIsValid(FeedInputValidity.Valid)
      setFeedInputText('')
      dispatch(todayActions.dataReceived({ publishers: result.publishers }))
      return
    }
    setFeedSearchResults(results.map(r => ({ ...r, status: FeedInputValidity.Valid })))
    setFeedInputIsValid(FeedInputValidity.HasResults)
  }

  const setFeedSearchResultsItemStatus = (sourceUrl: string, status: FeedInputValidity) => {
    setFeedSearchResults(existing => {
      // Amend by index to preserve order
      const itemIdx = existing.findIndex(item => item.feedUrl.url === sourceUrl)
      const newResults = [...existing]
      newResults[itemIdx] = {
        ...newResults[itemIdx],
        status
      }
      return newResults
    })
  }

  const removeSearchResultItem = (sourceUrl: string) => {
    const item = feedSearchResults.find(item => item.feedUrl.url === sourceUrl)
    if (item) {
      if (feedSearchResults.length > 1) {
        const others = feedSearchResults.filter(other => other !== item)
        setFeedSearchResults(others)
      } else {
        // Remove all results
        setFeedSearchResults([])
        setFeedInputIsValid(FeedInputValidity.Valid)
      }
    }
  }

  const onAddSource = async (sourceUrl: string) => {
    // Ask the backend
    setFeedSearchResultsItemStatus(sourceUrl, FeedInputValidity.Pending)
    const api = getBraveNewsAPI()
    const result = await api.subscribeToNewDirectFeed({ url: sourceUrl })
    const status = !result.isValidFeed
      ? FeedInputValidity.NotValid
      : result.isDuplicate
        ? FeedInputValidity.IsDuplicate
        : FeedInputValidity.Valid
    // Remove item if successful, as user shouldn't try to add more than once
    if (status === FeedInputValidity.Valid) {
      removeSearchResultItem(sourceUrl)
    } else {
      setFeedSearchResultsItemStatus(sourceUrl, status)
    }
    // Update state with new publisher list
    dispatch(todayActions.dataReceived({ publishers: result.publishers }))
  }

  return {
    userFeeds,
    feedInputIsValid,
    feedInputText,
    feedSearchResults,
    onRemoveDirectFeed,
    onChangeFeedInput,
    onSearchForSources,
    onAddSource
  }
}
