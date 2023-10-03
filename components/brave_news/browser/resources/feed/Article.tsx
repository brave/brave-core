// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { spacing } from '@brave/leo/tokens/css';
import { FeedItemMetadata, Article as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import * as React from 'react';
import styled from 'styled-components';
import Flex from '$web-common/Flex'
import { useLazyUnpaddedImageUrl } from '../shared/useUnpaddedImageUrl';
import ArticleMetaRow from './ArticleMetaRow';
import Card, { Title } from './Card';

interface Props {
  info: Info
  hideChannel?: boolean
}

const Container = styled(Card)`
  display: flex;
  flex-direction: column;
`

const ArticleImage = styled.img`
  min-width: 96px;
  height: 64px;

  object-fit: cover;
  object-position: top;

  border-radius: 6px;
`
export const openArticle = (article: FeedItemMetadata) => window.location.href = article.url.url

export default function Article({ info, hideChannel }: Props) {
  const { url: imageUrl, setElementRef } = useLazyUnpaddedImageUrl(info.data.image.paddedImageUrl?.url, { useCache: true })
  const url = info.data.url.url;

  return <Container ref={setElementRef} onClick={() => openArticle(info.data)}>
    <ArticleMetaRow article={info.data} hideChannel={hideChannel} />
    <Flex direction='row' gap={spacing.m} justify='space-between'>
      <Title>
        <a href={url}>{info.data.title}{('isDiscover' in info && info.isDiscover) && " (discovering)"}</a>
      </Title>
      <ArticleImage src={imageUrl} />
    </Flex>
  </Container>
}
