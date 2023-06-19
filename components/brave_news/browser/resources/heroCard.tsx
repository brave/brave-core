import styled from 'styled-components'
import { FeedItemMetadata } from '../../../../../out/Component/gen/brave/components/brave_news/common/brave_news.mojom.m'
import Card from './card'
import * as React from 'react'
import Image from './Image'

const BigText = styled.div`
  font: var(--leo-font-heading-h3);
`
const Description = styled.p`
  max-height: 150px;
  overflow: hidden;
`

export default function HeroCard({ article }: { article: FeedItemMetadata }) {
  return (
    <BigText>
      <Card onClick={() => window.open(article.url.url, '_blank')}>
        {article.image.paddedImageUrl?.url && <Image url={article.image.paddedImageUrl.url} />}
        Hero:
        <b>{article.title}</b>
        <div>Publisher: {article.publisherName}</div>
        <Description>{article.description}</Description>
      </Card>
    </BigText>
  )
}
