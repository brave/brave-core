// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import Card from './Card';
import { FeedItemMetadata, HeroArticle, Article as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import styled from 'styled-components';
import { color, font } from '@brave/leo/tokens/css';

interface Props {
  info: Info | HeroArticle
  isHero?: boolean
}

const Container = styled(Card)`
  cursor: pointer;
`

const Header = styled.h2<{ isHero?: boolean }>`
  all: unset;
  font: ${s => s.isHero ? font.primary.heading.h2 : font.primary.heading.h4};
`

const Publisher = styled.div`
  color: ${color.text.secondary};
`

const Description = styled.div`
  max-height: 100px;
  overflow: hidden;
`

export const openArticle = (article: FeedItemMetadata) => window.open(article.url.url, '_blank', 'noopener noreferrer')

export default function Article({ info, isHero }: Props) {
  return <Container onClick={() => openArticle(info.data)}>
    <Header isHero={isHero}>{isHero && 'Hero: '}{info.data.title}{('isDiscover' in info && info.isDiscover) && " (discovering)"}</Header>
    <Publisher>{info.data.publisherName} - {info.data.relativeTimeDescription}</Publisher>
    <Description>{info.data.description}</Description>
  </Container>
}
