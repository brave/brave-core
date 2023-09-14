// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import Card, { MetaInfo, Title } from './Card';
import { FeedItemMetadata, Article as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import styled from 'styled-components';
import { spacing } from '@brave/leo/tokens/css';
import { useLazyUnpaddedImageUrl } from '../../../../brave_new_tab_ui/components/default/braveNews/useUnpaddedImageUrl';

interface Props {
  info: Info
}

const Container = styled(Card)`
  display: flex;
  flex-direction: row;
  justify-content: space-between;
  gap: ${spacing.m};
`

const ArticleImage = styled.img`
  width: 120px;
  height: 80px;

  object-fit: cover;
  object-position: top;

  border-radius: 6px;
`

export const openArticle = (article: FeedItemMetadata) => window.open(article.url.url, '_blank', 'noopener noreferrer')

export default function Article({ info }: Props) {
  const { url, setElementRef } = useLazyUnpaddedImageUrl(info.data.image.paddedImageUrl?.url, { useCache: true })
  return <Container onClick={() => openArticle(info.data)} ref={setElementRef}>
    <div>
      <MetaInfo>{new URL(info.data.url.url).host} • {info.data.relativeTimeDescription}</MetaInfo>
      <Title>{info.data.title}{('isDiscover' in info && info.isDiscover) && " (discovering)"}</Title>
    </div>
    <ArticleImage src={url} />
  </Container>
}
