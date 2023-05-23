import * as React from 'react'
import Card from './card'
import { Publisher } from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import styled from 'styled-components'

const Row = styled.div`
  display: flex;
  flex-direction: row;
  gap: var(--leo-spacing-16);
`

function PublisherCard(props: { publisher: Publisher }) {
  return <Card>
    <h1>{props.publisher.publisherName}</h1>
  </Card>
}

export default function DiscoverCard(props: { sources: Publisher[] }) {
  return (
    <Card>
      <div>MAYBE YOU SHOULD SUBSCRIBE TO ONE OF THESE SUPER GREAT SOURCES?</div>
      <Row>
        {props.sources.map(s => <PublisherCard publisher={s} key={s.publisherId} />)}
      </Row>
    </Card>
  )
}
