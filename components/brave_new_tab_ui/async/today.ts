// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import AsyncActionHandler from '../../common/AsyncActionHandler'
import * as Background from '../../common/Background'
import * as Actions from '../actions/today_actions'
import { ApplicationState } from '../reducers'

function storeInHistoryState (data: Object) {
  const oldHistoryState = (typeof history.state === 'object') ? history.state : {}
  const newHistoryState = { ...oldHistoryState, ...data }
  history.pushState(newHistoryState, document.title)
}

import Messages = BraveToday.Messages
import MessageTypes = Background.MessageTypes.Today

const handler = new AsyncActionHandler()

handler.on(Actions.todayInit.getType(), async (store, dispatch) => {
  try {
    const [{feed}, {publishers}] = await Promise.all([
      Background.send<Messages.GetFeedResponse>(MessageTypes.getFeed),
      Background.send<Messages.GetPublishersResponse>(MessageTypes.getPublishers)
    ])
    dispatch(Actions.dataReceived({feed, publishers}))
  } catch (e) {
    console.error('error receiving feed', e)
    dispatch(Actions.errorGettingDataFromBackground(e))
  }
})

handler.on(Actions.ensureSettingsData.getType(), async (store, dispatch) => {
  const state = store.getState() as ApplicationState
  if (state.today.publishers && Object.keys(state.today.publishers).length) {
    return
  }
  const { publishers } = await Background.send<Messages.GetPublishersResponse>(MessageTypes.getPublishers)
  dispatch(Actions.dataReceived({publishers}))
})

handler.on<Actions.ReadFeedItemPayload>(Actions.readFeedItem.getType(), async (store, dispatch, payload) => {
  const state = store.getState() as ApplicationState
  const todayPageIndex = state.today.currentPageIndex
  // remember article so we can scroll to it on "back" navigation
  storeInHistoryState({ todayArticle: payload, todayPageIndex })
  // visit article url
  // @ts-ignore
  window.location = payload.url
})

handler.on<Actions.SetPublisherPrefPayload>(Actions.setPublisherPref.getType(), async (store, dispatch, payload) => {
  const { publisherId, enabled } = payload
  const { publishers } = await Background.send<Messages.SetPublisherPrefResponse, Messages.SetPublisherPrefPayload>(MessageTypes.setPublisherPref, {
    publisherId,
    enabled
  })
  dispatch(Actions.dataReceived({publishers}))
  dispatch(Actions.checkForUpdate())
})

handler.on(Actions.checkForUpdate.getType(), async function (store, dispatch) {
  const isUpdateAvailable = await Background.send<Messages.IsFeedUpdateAvailableResponse>(MessageTypes.isFeedUpdateAvailable)
  dispatch(Actions.isUpdateAvailable(isUpdateAvailable))
})

export default handler.middleware