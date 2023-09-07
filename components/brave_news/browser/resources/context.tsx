// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BraveNewsController, Channel, FeedV2, Publisher, Signal
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import * as React from 'react'
import usePromise from '../../../brave_new_tab_ui/hooks/usePromise'


export interface InspectContext {
  feed: FeedV2 | undefined,
  publishers: { [key: string]: Publisher },
  channels: { [key: string]: Channel },
  signals: { [key: string]: Signal },
  truncate: number,
  setTruncate: (value: number) => void
}

export const api = BraveNewsController.getRemote();

const Context = React.createContext<InspectContext>({
  publishers: {},
  channels: {},
  signals: {},
  feed: undefined,
  truncate: 0,
  setTruncate: () => { }
})

export const useInspectContext = () => {
  return React.useContext(Context);
}

export default function InspectContext(props: React.PropsWithChildren<{}>) {
  const { result: publishers } = usePromise(() => api.getPublishers().then(p => p.publishers as { [key: string]: Publisher }), [])
  const { result: channels } = usePromise(() => api.getChannels().then(c => c.channels as { [key: string]: Channel }), [])
  const { result: feed } = usePromise(() => api.getFeedV2().then(r => r.feed), [])
  const { result: signals } = usePromise(() => api.getSignals().then(r => r.signals), [feed]);
  const [truncate, setTruncate] = React.useState(parseInt(localStorage.getItem('truncate') || '') || 250)
  const setAndSaveTruncate = React.useCallback((value: number) => {
    localStorage.setItem('truncate', value.toString())
    setTruncate(value)
  }, [])
  const context = React.useMemo<InspectContext>(() => ({
    publishers: publishers ?? {},
    channels: channels ?? {},
    signals: signals ?? {},
    feed,
    truncate,
    setTruncate: setAndSaveTruncate
  }), [publishers, channels, feed, truncate, setAndSaveTruncate])

  return <Context.Provider value={context}>
    {props.children}
  </Context.Provider>
}
