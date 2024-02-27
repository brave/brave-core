// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { HeroArticle as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import * as React from 'react';
import { useLazyUnpaddedImageUrl } from '../shared/useUnpaddedImageUrl';
import ArticleMetaRow from './ArticleMetaRow';
import Card, { BraveNewsLink, LargeImage, Title, braveNewsCardClickHandler } from './Card';
import styled from 'styled-components';
import { spacing } from '@brave/leo/tokens/css';
import { useBraveNews } from '../shared/Context';

interface Props {
  info: Info
  feedDepth?: number
}

const Container = styled(Card)`
  display: flex;
  flex-direction: column;
  gap: ${spacing.s};

  & > ${LargeImage} {
    margin-bottom: ${spacing.l};
  }
`

export default function HeroArticle({ info, feedDepth }: Props) {
  const { reportVisit } = useBraveNews()
  const { url, setElementRef } = useLazyUnpaddedImageUrl(info.data.image.paddedImageUrl?.url ?? info.data.image.imageUrl?.url, {
    useCache: true,
    rootElement: document.body,
    rootMargin: '500px 0px'
  })
  return <Container
    onClick={() => {
      braveNewsCardClickHandler(info.data.url.url)
      if (feedDepth !== undefined) {
        reportVisit(feedDepth)
      }
    }} ref={setElementRef}>
    <LargeImage src={url} />
    <ArticleMetaRow article={info.data} />
    <Title>
      <BraveNewsLink
        href={info.data.url.url}
        feedDepth={feedDepth}
      >
        {info.data.title}
      </BraveNewsLink>
    </Title>
  </Container>
}
