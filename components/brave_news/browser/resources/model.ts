import { FeedItemMetadata, Publisher } from "gen/brave/components/brave_news/common/brave_news.mojom.m";

export type ArticleElements = {
  type: 'hero',
  article: FeedItemMetadata
} | {
  type: 'inline',
  article: FeedItemMetadata,
  isDiscover: boolean
}

export type Elements = ArticleElements | {
  type: 'advert'
} | {
  type: 'discover'
  publishers: Publisher[]
} | {
  type: 'cluster',
  clusterType: {
    type: 'channel',
    id: string
  } | {
    type: 'topic',
    id: string
  }
  elements: ArticleElements[]
}
