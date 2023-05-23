import styled from 'styled-components'
import {
  FeedItemMetadata
} from '../../../../../out/Component/gen/brave/components/brave_news/common/brave_news.mojom.m'
import Card from './card'
import * as React from 'react'
import Image from './Image'

const Row = styled.div`
  display: flex;
  flex-direction: row;
`

const ImageContainer = styled.div`
  width: 128px;
`

export default function InlineCard({ article, isDiscover }: { article: FeedItemMetadata, isDiscover: boolean }) {
  return (
    <Card onClick={() => window.open(article.url.url, '_blank')}>
      <Row>
        {article.image.paddedImageUrl?.url && <ImageContainer>
          <Image url={article.image.paddedImageUrl.url} />
        </ImageContainer>}
        <div>
          Inline:
          <b>{article.title}</b> ({isDiscover ? 'discovering' : 'normal'})
          <div>Publisher: {article.publisherName}</div>
          <p>{article.description}</p>
        </div>
      </Row>
    </Card>
  )
}
