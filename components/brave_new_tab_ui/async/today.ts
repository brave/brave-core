// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import AsyncActionHandler from '../../common/AsyncActionHandler'
import * as Actions from '../actions/today_actions'
import { ApplicationState } from '../reducers'
import { saveIsBraveTodayOptedIn } from '../api/preferences'
import getBraveNewsController, * as BraveNews from '../api/brave_news'

function storeInHistoryState (data: Object) {
  const oldHistoryState = (typeof history.state === 'object') ? history.state : {}
  const newHistoryState = { ...oldHistoryState, ...data }
  history.pushState(newHistoryState, document.title)
}

const handler = new AsyncActionHandler()

handler.on(Actions.interactionBegin.getType(), async () => {
  getBraveNewsController().onInteractionSessionStarted()
})

handler.on(
  [Actions.interactionBegin.getType(), Actions.refresh.getType()],
  async (store) => {
    try {
      const [{ feed }, { publishers }] = await Promise.all([
        getBraveNewsController().getFeed(),
        getBraveNewsController().getPublishers()
      ])

      store.dispatch(Actions.dataReceived({ feed, publishers }))
    } catch (e) {
      console.error('error receiving feed', e)
      store.dispatch(Actions.errorGettingDataFromBackground(e))
    }
  }
)

handler.on(Actions.optIn.getType(), async () => {
  saveIsBraveTodayOptedIn(true)
})

handler.on(Actions.ensureSettingsData.getType(), async (store) => {
  const state = store.getState() as ApplicationState
  if (state.today.publishers && Object.keys(state.today.publishers).length) {
    return
  }
  const { publishers } = await getBraveNewsController().getPublishers()
  store.dispatch(Actions.dataReceived({ publishers }))
})

handler.on<Actions.ReadFeedItemPayload>(Actions.readFeedItem.getType(), async (store, payload) => {
  const state = store.getState() as ApplicationState
  getBraveNewsController().onSessionCardVisitsCountChanged(state.today.cardsVisited)
  if (payload.isPromoted) {
    const promotedArticle = payload.item.promotedArticle
    if (!promotedArticle) {
      console.error('Brave News: readFeedItem payload with invalid promoted article', payload)
      return
    }
    if (!payload.promotedUUID) {
      console.error('Brave News: invalid promotedUUID for readFeedItem', payload)
      return
    }
    getBraveNewsController().onPromotedItemVisit(payload.promotedUUID, promotedArticle.creativeInstanceId)
  }
  const data = payload.item.article?.data || payload.item.promotedArticle?.data ||
      payload.item.deal?.data
  if (!data) {
    console.error('Brave News: readFeedItem payload item not present', payload)
    return
  }
  if (!payload.openInNewTab) {
    // remember article so we can scroll to it on "back" navigation
    // TODO(petemill): Type this history.state data and put in an API module
    // (see `reducers/today`).
    storeInHistoryState({
      todayArticle: data,
      todayPageIndex: state.today.currentPageIndex,
      todayCardsVisited: state.today.cardsVisited
    })
    // visit article url
    window.location.href = data.url.url
  } else {
    window.open(data.url.url, '_blank')
  }
})

handler.on<Actions.PromotedItemViewedPayload>(Actions.promotedItemViewed.getType(), async (store, payload) => {
  if (!payload.item.promotedArticle) {
    console.error('Brave News: promotedItemViewed invalid promoted article', payload)
    return
  }
  getBraveNewsController().onPromotedItemView(payload.uuid, payload.item.promotedArticle.creativeInstanceId)
})

handler.on<number>(Actions.feedItemViewedCountChanged.getType(), async (store, payload) => {
  const state = store.getState() as ApplicationState
  getBraveNewsController().onSessionCardViewsCountChanged(state.today.cardsViewed)
})

handler.on<Actions.RemoveDirectFeedPayload>(Actions.removeDirectFeed.getType(), async (store, payload) => {
  getBraveNewsController().removeDirectFeed(payload.directFeed.publisherId)
  window.setTimeout(() => {
    store.dispatch(Actions.checkForUpdate())
  }, 3000)
})

handler.on<Actions.SetPublisherPrefPayload>(Actions.setPublisherPref.getType(), async (store, payload) => {
  const { publisherId, enabled } = payload
  let userStatus = (enabled === null)
    ? BraveNews.UserEnabled.NOT_MODIFIED
    : enabled
      ? BraveNews.UserEnabled.ENABLED
      : BraveNews.UserEnabled.DISABLED
  getBraveNewsController().setPublisherPref(publisherId, userStatus)
  // Refreshing of content after prefs changed is throttled, so wait
  // a while before seeing if we have new content yet.
  // This doesn't have to be exact since we often check for update when
  // opening or scrolling through the feed.
  window.setTimeout(() => {
    store.dispatch(Actions.checkForUpdate())
  }, 3000)
})

handler.on(Actions.checkForUpdate.getType(), async function (store) {
  const state = store.getState() as ApplicationState
  if (!state.today.feed || !state.today.feed.hash) {
    store.dispatch(Actions.isUpdateAvailable({ isUpdateAvailable: true }))
    return
  }
  const hash = state.today.feed.hash
  const isUpdateAvailable: {isUpdateAvailable: boolean} = await getBraveNewsController().isFeedUpdateAvailable(hash)
  store.dispatch(Actions.isUpdateAvailable(isUpdateAvailable))
})

handler.on(Actions.resetTodayPrefsToDefault.getType(), async function (store) {
  getBraveNewsController().clearPrefs()
  const { publishers } = await getBraveNewsController().getPublishers()
  store.dispatch(Actions.dataReceived({ publishers }))
  store.dispatch(Actions.checkForUpdate())
})

handler.on(Actions.anotherPageNeeded.getType(), async function (store) {
  store.dispatch(Actions.checkForUpdate())
})

handler.on<Actions.VisitDisplayAdPayload>(Actions.visitDisplayAd.getType(), async function (store, payload) {
  const state = store.getState() as ApplicationState
  const todayPageIndex = state.today.currentPageIndex
  getBraveNewsController().onDisplayAdVisit(payload.ad.uuid, payload.ad.creativeInstanceId)
  const destinationUrl = payload.ad.targetUrl.url
  if (!payload.openInNewTab) {
    // Remember display ad location so we can scroll to it on "back" navigation
    // We remember position and not ad ID since it can be a different ad on
    // a new page load.
    // TODO(petemill): Type this history.state data and put in an API module
    // (see `reducers/today`).
    storeInHistoryState({
      todayAdPosition: todayPageIndex,
      todayPageIndex,
      todayCardsVisited: state.today.cardsVisited
    })
    // visit article url
    window.location.href = destinationUrl
  } else {
    window.open(destinationUrl, '_blank')
  }
})

handler.on<Actions.DisplayAdViewedPayload>(Actions.displayAdViewed.getType(), async (store, item) => {
  getBraveNewsController().onDisplayAdView(item.ad.uuid, item.ad.creativeInstanceId)
})

export default handler.middleware
