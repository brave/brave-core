// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import {
  FeedItemMetadata,
  Signal
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import Card from './card'
import * as React from 'react'
import Image from './Image'

const Row = styled.div`
  display: flex;
  flex-direction: row;
  gap: 16px;
`

const ImageContainer = styled.div`
  --width: 128px;

  min-width: var(--width);
  max-width: var(--width);
`

const Description = styled.p`
  max-height: 50px;
  overflow: hidden;
`

export default function InlineCard({
  article,
  isDiscover,
  signal
}: {
  article: FeedItemMetadata
  isDiscover: boolean
  signal: Signal
}) {
  return (
    <Card onClick={() => window.open(article.url.url, '_blank')}>
      <Row>
        {article.image.paddedImageUrl?.url && (
          <ImageContainer>
            <Image url={article.image.paddedImageUrl.url} />
          </ImageContainer>
        )}
        <div>
          Inline:
          <b>{article.title}</b> ({isDiscover ? 'discovering' : 'normal'})
          <div>Publisher: {article.publisherName}</div>
          <pre>({JSON.stringify(signal, null, 4)})</pre>
          <Description>{article.description}</Description>
        </div>
      </Row>
    </Card>
  )
}
