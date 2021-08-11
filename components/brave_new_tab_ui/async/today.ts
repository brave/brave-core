// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import AsyncActionHandler from '../../common/AsyncActionHandler'
import * as Actions from '../actions/today_actions'
import { ApplicationState } from '../reducers'
import { saveIsBraveTodayOptedIn } from '../api/preferences'
import braveNewsController from '../api/brave_news/brave_news_proxy'

type MojoTime = {
  internalValue: number
}
/**
 * Converts a mojo time to a JS time.
 */
 function convertMojoTimeToJS (mojoTime: MojoTime): Date {
  // The JS Date() is based off of the number of milliseconds since the
  // UNIX epoch (1970-01-01 00::00:00 UTC), while |internalValue| of the
  // base::Time (represented in mojom.Time) represents the number of
  // microseconds since the Windows FILETIME epoch (1601-01-01 00:00:00 UTC).
  // This computes the final JS time by computing the epoch delta and the
  // conversion from microseconds to milliseconds.
  const windowsEpoch = Date.UTC(1601, 0, 1, 0, 0, 0, 0);
  const unixEpoch = Date.UTC(1970, 0, 1, 0, 0, 0, 0);
  // |epochDeltaInMs| equals to base::Time::kTimeTToMicrosecondsOffset.
  const epochDeltaInMs = unixEpoch - windowsEpoch;
  const timeInMs = Number(mojoTime.internalValue) / 1000;

  return new Date(timeInMs - epochDeltaInMs);
};

// TODO(petemill): This is temporary until we remove original types
function convertArticleFromMojom(item: any): BraveToday.Article {
  try {
    return {
      content_type: 'article',
      category: item.data.category_name,
      publish_time: convertMojoTimeToJS(item.data.publishTime),
      title: item.data.title,
      description: item.data.description,
      url: item.data.url.url,
      url_hash: item.data.url.url,
      padded_img: item.data.image.paddedImageUrl.url,
      img: item.data.image.paddedImageUrl.url,
      publisher_id: item.data.publisherId,
      publisher_name: item.data.publisherName,
      score: item.data.score,
      relative_time: item.data.relativeTimeDescription // TODO
    }
  }
  catch (e) {
    console.log('error converting item', item)
    throw e
  }
}
function convertDealFromMojom(item: any): BraveToday.Deal {
  return {
    ...convertArticleFromMojom(item),
    content_type: 'product',
    offers_category: item.offersCategory
  }
}
function convertPromotedArticleFromMojom(item: any): BraveToday.PromotedArticle {
  return {
    ...convertArticleFromMojom(item),
    content_type: 'brave_partner',
    creative_instance_id: item.creativeInstanceId
  }
}
function convertPageFromMojom(page: any): BraveToday.Page {
  return {
    articles: page.articles.map(convertArticleFromMojom),
    randomArticles: page.randomArticles.map(convertArticleFromMojom),
    itemsByCategory: page.itemsByCategory
      ? {
        categoryName: page.itemsByCategory.categoryName,
        items: page.itemsByCategory.articles.map(convertArticleFromMojom)
      }
      : undefined,
    itemsByPublisher: page.itemsByPublisher
    ? {
      name: page.itemsByPublisher.publisherId,
      items: page.itemsByPublisher.articles.map(convertArticleFromMojom)
    }
    : undefined,
    deals: page.deals ? page.deals.map(convertDealFromMojom) : undefined,
    promotedArticle: page.promotedArticle ? convertPromotedArticleFromMojom(page.promotedArticle) : undefined
  }
}
function convertFromMojomFeed(feed: any): BraveToday.Feed {
  return {
    hash: feed.hash,
    featuredArticle: feed.featuredArticle ? convertArticleFromMojom(feed.featuredArticle) : undefined,
    pages: feed.pages.map((page: any) => convertPageFromMojom(page)),
  }
}

function convertFromMojomPublishers(mPublishers: any): BraveToday.Publishers {
  const publishers = {}
  for (const publisherId in mPublishers) {
    let orig = mPublishers[publisherId]
    publishers[publisherId] = {
      publisher_id: publisherId,
      publisher_name: orig.publisherName,
      category: orig.categoryName,
      enabled: orig.isEnabled,
      user_enabled: orig.userEnabledStatus === 0 ? null : orig.userEnabledStatus === 1 ? true : false
    }
  }
  return publishers
}


function storeInHistoryState (data: Object) {
  const oldHistoryState = (typeof history.state === 'object') ? history.state : {}
  const newHistoryState = { ...oldHistoryState, ...data }
  history.pushState(newHistoryState, document.title)
}

const handler = new AsyncActionHandler()

handler.on(Actions.todayInit.getType(), async (store, payload) => {
  // Let backend know that a UI with today is open, so that it can
  // pre-fetch the feed if it is not already in a cache.
  // await Background.send(MessageTypes.indicatingOpen)
})

handler.on(Actions.interactionBegin.getType(), async () => {
  braveNewsController.onInteractionSessionStarted()
})

handler.on(
  [Actions.interactionBegin.getType(), Actions.refresh.getType()],
  async (store) => {
    try {
      const [{ feed }, { publishers }] = await Promise.all([
        braveNewsController.getFeed(),
        braveNewsController.getPublishers()
      ])

      store.dispatch(Actions.dataReceived({ feed: convertFromMojomFeed(feed), publishers: convertFromMojomPublishers(publishers) }))
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
  const { publishers } = await braveNewsController.getPublishers()
  store.dispatch(Actions.dataReceived({ publishers: convertFromMojomPublishers(publishers) }))
})

handler.on<Actions.ReadFeedItemPayload>(Actions.readFeedItem.getType(), async (store, payload) => {
  const state = store.getState() as ApplicationState
  braveNewsController.onSessionCardVisitsCountChanged(state.today.cardsVisited)
  if (payload.isPromoted) {
    const promotedArticle = payload.item as BraveToday.PromotedArticle
    braveNewsController.onPromotedItemVisit(payload.promotedUUID, promotedArticle.creative_instance_id)
  }
  if (!payload.openInNewTab) {
    // remember article so we can scroll to it on "back" navigation
    // TODO(petemill): Type this history.state data and put in an API module
    // (see `reducers/today`).
    storeInHistoryState({
      todayArticle: payload.item,
      todayPageIndex: state.today.currentPageIndex,
      todayCardsVisited: state.today.cardsVisited
    })
    // visit article url
    window.location.href = payload.item.url
  } else {
    window.open(payload.item.url, '_blank')
  }
})

handler.on<Actions.PromotedItemViewedPayload>(Actions.promotedItemViewed.getType(), async (store, payload) => {
  braveNewsController.onPromotedItemView(payload.uuid, payload.item.creative_instance_id)
})

handler.on<number>(Actions.feedItemViewedCountChanged.getType(), async (store, payload) => {
  const state = store.getState() as ApplicationState
  braveNewsController.onSessionCardViewsCountChanged(state.today.cardsViewed)
})

handler.on<Actions.SetPublisherPrefPayload>(Actions.setPublisherPref.getType(), async (store, payload) => {
  const { publisherId, enabled } = payload
  let userStatus = (enabled === null)
    ? braveNews.mojom.UserEnabled.NOT_MODIFIED
    : enabled === true
      ? braveNews.mojom.UserEnabled.ENABLED
      : braveNews.mojom.UserEnabled.DISABLED
  braveNewsController.setPublisherPref(publisherId, userStatus)
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
  const isUpdateAvailable: {isUpdateAvailable: boolean} = await braveNewsController.isFeedUpdateAvailable(hash)
  store.dispatch(Actions.isUpdateAvailable(isUpdateAvailable))
})

handler.on(Actions.resetTodayPrefsToDefault.getType(), async function (store) {
  braveNewsController.clearPrefs()
  const { publishers } = await braveNewsController.getPublishers()
  store.dispatch(Actions.dataReceived({ publishers }))
  store.dispatch(Actions.checkForUpdate())
})

handler.on(Actions.anotherPageNeeded.getType(), async function (store) {
  store.dispatch(Actions.checkForUpdate())
})

handler.on<Actions.VisitDisplayAdPayload>(Actions.visitDisplayAd.getType(), async function (store, payload) {
  const state = store.getState() as ApplicationState
  const todayPageIndex = state.today.currentPageIndex
  braveNewsController.onDisplayAdVisit(payload.ad.uuid, payload.ad.creativeInstanceId)
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
  braveNewsController.onDisplayAdView(item.ad.uuid, item.ad.creativeInstanceId)
})

export default handler.middleware
