// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import Flex from '$web-common/Flex';
import { spacing } from '@brave/leo/tokens/css';
import { Article as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import * as React from 'react';
import styled from 'styled-components';
import { useBraveNews } from '../shared/Context';
import { useLazyUnpaddedImageUrl } from '../shared/useUnpaddedImageUrl';
import ArticleMetaRow from './ArticleMetaRow';
import Card, { BraveNewsLink, SmallImage, Title, braveNewsCardClickHandler } from './Card';

interface Props {
  info: Info
  hideChannel?: boolean
  feedDepth?: number
}

const Container = styled(Card)`
  display: flex;
  flex-direction: column;
  gap: ${spacing.s};
  padding-top: ${spacing.l};
`

export default function Article({ info, hideChannel, feedDepth }: Props) {
  const { reportVisit } = useBraveNews()
  const { url: imageUrl, setElementRef } = useLazyUnpaddedImageUrl(info.data.image.paddedImageUrl?.url ?? info.data.image.imageUrl?.url, {
    useCache: true,
    rootMargin: '500px 0px'
  })
  const url = info.data.url.url;

  return <Container
      ref={setElementRef}
      onClick={() => {
        braveNewsCardClickHandler(url)
        if (feedDepth !== undefined) {
          reportVisit(feedDepth)
        }
      }}
    >
    <ArticleMetaRow article={info.data} hideChannel={hideChannel} />
    <Flex direction='row' gap={spacing.xl} justify='space-between' align='start'>
      <Title>
        <BraveNewsLink
          href={url}
          feedDepth={feedDepth}
        >
          {info.data.title}
        </BraveNewsLink>
      </Title>
      <SmallImage src={imageUrl} />
    </Flex>
  </Container>
}
