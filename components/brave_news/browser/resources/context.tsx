// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BraveNewsController,
  FeedV2,
  Signal
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import * as React from 'react'
import usePromise from '$web-common/usePromise'
import { BraveNewsContextProvider } from './shared/Context';
export type FeedType = 'all' | `publishers/${string}` | `channels/${string}`

export interface InspectContext {
  currentFeed: FeedType,
  setCurrentFeed: (type: FeedType) => void,
  feed: FeedV2 | undefined,
  signals: { [key: string]: Signal },
  truncate: number,
  setTruncate: (value: number) => void
}

export const api = BraveNewsController.getRemote();

const Context = React.createContext<InspectContext>({
  currentFeed: 'all',
  setCurrentFeed: () => {},
  signals: {},
  feed: undefined,
  truncate: 0,
  setTruncate: () => { }
})

export const useInspectContext = () => {
  return React.useContext(Context);
}

export default function InspectContext(props: React.PropsWithChildren<{}>) {
  const [currentFeed, setCurrentFeed] = React.useState<InspectContext['currentFeed']>('all')
  const { result: feed } = usePromise(() => {
    let promise: Promise<{ feed: FeedV2 }>
    if (currentFeed.startsWith('publishers/')) {
      promise = api.getPublisherFeed(currentFeed.split('/')[1]);
    } else if (currentFeed.startsWith('channels/')) {
      promise = api.getChannelFeed(currentFeed.split('/')[1])
    } else {
      promise = api.getFeedV2()
    }
    return promise.then(f => f.feed)
  }, [currentFeed])
  const { result: signals } = usePromise(() => api.getSignals().then(r => r.signals), [feed]);
  const [truncate, setTruncate] = React.useState(parseInt(localStorage.getItem('truncate') || '') || 250)
  const setAndSaveTruncate = React.useCallback((value: number) => {
    localStorage.setItem('truncate', value.toString())
    setTruncate(value)
  }, [])
  const context = React.useMemo<InspectContext>(() => ({
    currentFeed,
    setCurrentFeed,
    signals: signals ?? {},
    feed,
    truncate,
    setTruncate: setAndSaveTruncate
  }), [signals, feed, truncate, setAndSaveTruncate, currentFeed])

  return <BraveNewsContextProvider>
    <Context.Provider value={context}>
      {props.children}
    </Context.Provider>
  </BraveNewsContextProvider>
}
