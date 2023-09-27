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
import Flex from '../../../../brave_new_tab_ui/components/Flex';
import Button from '@brave/leo/react/button';
import Icon from '@brave/leo/react/icon';

interface Props {
  info: Info
  hideChannel?: boolean
}

const Container = styled(Card)`
  display: flex;
  flex-direction: column;
`

const ArticleImage = styled.img`
  width: 96px;
  min-width: 96px;
  height: 64px;

  object-fit: cover;
  object-position: top;

  border-radius: 6px;
`

const MenuButton = styled(Button)`
  flex-grow: 0;
  --leo-button-padding: ${spacing.s};
`

export const openArticle = (article: FeedItemMetadata) => window.open(article.url.url, '_blank', 'noopener noreferrer')

export default function Article({ info, hideChannel }: Props) {
  const { url, setElementRef } = useLazyUnpaddedImageUrl(info.data.image.paddedImageUrl?.url, { useCache: true })

  return <Container onClick={() => openArticle(info.data)} ref={setElementRef}>
    <Flex direction='row' gap={spacing.m} justify='space-between'>
      <MetaInfo article={info.data} hideChannel={hideChannel} />
      <MenuButton kind='plain-faint' onClick={e => {
        e.preventDefault()
        e.stopPropagation()
        e.stopImmediatePropagation()
      }}><Icon name='more-horizontal' /></MenuButton>
    </Flex>
    <Flex direction='row' gap={spacing.m}>
      <Title>{info.data.title}{('isDiscover' in info && info.isDiscover) && " (discovering)"}</Title>
      <ArticleImage src={url} />
    </Flex>
  </Container>
}
