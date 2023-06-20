import * as React from 'react'
import { render } from 'react-dom'

import { BraveNewsController, Channel, FeedItem } from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import styled from 'styled-components'
import { Info, generateFeed } from './buildFeed'
import Elements from './Elements'
import Config from './configureWeights'

const Container = styled.div`
  display: flex;
  flex-direction: column;
  gap: var(--leo-spacing-8);
  max-width: 800px;
  margin: 0 auto;
`

export const api = BraveNewsController.getRemote()
window['api'] = api

const signalsPromise = api.getSignals().then((r) => r.signals)
const feedPromise = api.getRawFeed().then((r) => r.items) as Promise<FeedItem[]>
const channelsPromise = api.getChannels().then((r) => Object.values(r.channels) as Channel[])
const suggestionsPromise = api
  .getSuggestedPublisherIds()
  .then((r) => r.suggestedPublisherIds)
const publishersPromise = api.getPublishers().then((r) => r.publishers)

const articlesPromise = (async () => {
  const feed = await feedPromise
  return feed.filter((i) => i.article).map((i) => i.article!.data)
})()

const suggestedPublishersPromise = (async () => {
  const publishers = await publishersPromise
  const suggestions = await suggestionsPromise
  return suggestions.map((s) => publishers[s])
})()

const infoPromise: Promise<Info> = (async () => {
  return {
    publishers: await publishersPromise,
    suggested: await suggestedPublishersPromise,
    channels: await channelsPromise,
    articles: await articlesPromise,
    signals: await signalsPromise
  }
})()

infoPromise.then(i => window['info'] = i)

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
  const info = usePromise(infoPromise, undefined, [])

  const feedElements = React.useMemo(() => info && generateFeed(info), [info]) ?? []

  return (
    <div
      style={{
        background: 'var(--leo-color-page-background)',
        color: 'var(--leo-color-text-primary)',
        padding: 'var(--leo-spacing-8)'
      }}
    >
      <Container>
        <div>
          <Config/>
        </div>
        <Elements elements={feedElements} signals={info?.signals ?? {}} />
      </Container>
    </div>
  )
}

render(<App />, document.getElementById('root'))
