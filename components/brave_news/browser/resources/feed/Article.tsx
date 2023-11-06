// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import Flex from '$web-common/Flex';
import SecureLink, { validateScheme } from '$web-common/SecureLink';
import { spacing } from '@brave/leo/tokens/css';
import { FeedItemMetadata, Article as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import * as React from 'react';
import styled from 'styled-components';
import { useLazyUnpaddedImageUrl } from '../shared/useUnpaddedImageUrl';
import ArticleMetaRow from './ArticleMetaRow';
import Card, { SmallImage, Title } from './Card';

interface Props {
  info: Info
  hideChannel?: boolean
}

const Container = styled(Card)`
  display: flex;
  flex-direction: column;
`

export const openArticle = (article: FeedItemMetadata) => {
  const href = article.url.url
  validateScheme(href)
  window.location.href = href
}

export default function Article({ info, hideChannel }: Props) {
  const { url: imageUrl, setElementRef } = useLazyUnpaddedImageUrl(info.data.image.paddedImageUrl?.url ?? info.data.image.imageUrl?.url, { useCache: true })
  const url = info.data.url.url;

  return <Container ref={setElementRef} onClick={() => openArticle(info.data)}>
    <ArticleMetaRow article={info.data} hideChannel={hideChannel} />
    <Flex direction='row' gap={spacing.m} justify='space-between'>
      <Title>
        <SecureLink href={url}>{info.data.title}{('isDiscover' in info && info.isDiscover) && " (discovering)"}</SecureLink>
      </Title>
      <SmallImage src={imageUrl} />
    </Flex>
  </Container>
}
