// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'

import {
  BraveNewsController, Channel
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import styled from 'styled-components'
import { Info, generateFeed } from './buildFeed'
import Elements from './Elements'
import Config from './configureWeights'
import SignalDetails from './signalDetails'
import Button from '@brave/leo/react/button'

const Container = styled.div`
  display: flex;
  flex-direction: column;
  gap: var(--leo-spacing-8);
  max-width: 800px;
  margin: 0 auto;

  :global body {
    overflow-x: hidden;
  }
`

export const api = BraveNewsController.getRemote();

const signalsPromise = api.getSignals().then((r) => r.signals)
const feedPromise = api.getRawFeed().then((r) => r.items)
const channelsPromise = api
  .getChannels()
  .then((r) => Object.values(r.channels));
const suggestionsPromise = api
  .getSuggestedPublisherIds()
  .then((r) => r.suggestedPublisherIds);
const publishersPromise = api.getPublishers().then((r) => r.publishers);

const articlesPromise = (async () => {
  const feed = await feedPromise
  return feed.filter((i) => i.article).map((i) => i.article!.data)
})();

const suggestedPublishersPromise = (async () => {
  const publishers = await publishersPromise
  const suggestions = await suggestionsPromise
  return suggestions.map((s) => publishers[s])
})();

const infoPromise = (async (): Promise<Info> => {
  return {
    publishers: await publishersPromise,
    suggested: await suggestedPublishersPromise,
    channels: await channelsPromise as Channel[],
    articles: await articlesPromise,
    signals: await signalsPromise
  }
})();

// Expose the api and info on the window object, so we can use them from the
// console.
(window as any).api = api
infoPromise.then((i) => ((window as any).info = i))

function usePromise<T>(promise: Promise<T>, defaultValue: T, deps: any[]) {
  const [result, setResult] = React.useState<T | undefined>(defaultValue)
  React.useEffect(() => {
    let cancelled = false
    promise.then((r) => !cancelled && setResult(r))
    return () => {
      cancelled = true
    }
  }, deps)

  return result
}

function App() {
  const [showSignals, setShowSignals] = React.useState(true)
  const info = usePromise(infoPromise, undefined, [])

  const feedElements =
    React.useMemo(() => info && generateFeed(info), [info]) ?? []

  return (
    <div
      style={{
        background: 'var(--leo-color-page-background)',
        color: 'var(--leo-color-text-primary)',
        padding: 'var(--leo-spacing-8)'
      }}
    >
      <Container>
        <Button onClick={() => setShowSignals(s => !s)}>{showSignals ? 'view feed' : 'view signals'}</Button>
        {showSignals
          ? <SignalDetails signals={info?.signals ?? {}} publishers={info?.publishers ?? {}} />
          : <>
            <div>
              <Config />
            </div>
            <Elements elements={feedElements} signals={info?.signals ?? {}} />
          </>}
      </Container>
    </div>
  )
}

render(<App />, document.getElementById('root'))
