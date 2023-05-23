import * as React from 'react'
import { FeedItemMetadata } from '../../../../../out/Component/gen/brave/components/brave_news/common/brave_news.mojom.m'
import Card from './card'

export default function ClusterBlock(props: {
  channelOrTopic: string
  articles: FeedItemMetadata[]
}) {
  return <Card>CLUSTER BLOCK FOR {props.channelOrTopic} PLACEHOLDER</Card>
}
