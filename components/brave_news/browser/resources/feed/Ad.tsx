// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import usePromise from '$web-common/usePromise';
import Button from '@brave/leo/react/button';
import { font, spacing } from '@brave/leo/tokens/css';
import { PromotedArticle } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import * as React from 'react';
import styled from 'styled-components';
import getBraveNewsController from '../shared/api';
import { MetaInfoContainer } from './ArticleMetaRow';
import Card, { LargeImage, Title } from './Card';
import SecureLink from '../../../../common/SecureLink';
import { useUnpaddedImageUrl } from '../shared/useUnpaddedImageUrl';

interface Props {
  info: PromotedArticle
}

const Container = styled(Card)`
  display: flex;
  flex-direction: column;
  gap: ${spacing.m}
`

const BatAdLabel = styled.a`
  padding: 0 2px;

  border: 1px solid rgba(var(--bn-text-base), 0.3);
  border-radius: 3px;

  text-decoration: none;

  color: rgba(var(--bn-text-base, 0.7));
  font: ${font.primary.small.regular};
`

const CtaButton = styled(Button)`
  align-self: flex-start;
`

export default function Advert(props: Props) {
  const { result } = usePromise(() => getBraveNewsController()
    .getDisplayAd().then(r => r.ad ?? {
      title: '10 reasons why technica recreated the sound of old classics.',
      description: 'Technica',
      creativeInstanceId: '1234',
      ctaText: 'Get Started',
      targetUrl: { url: 'https://www.brave.com' },
      image: { imageUrl: undefined, paddedImageUrl: { url: 'https://pcdn.bravesoftware.com/brave-today/favicons/f35d56d075b3015a7eef41273d56f4f9665b3749b2318abf531a20722b824bb3.jpg.pad' } },
      dimensions: '1x3',
      uuid: '0abc'
    }), [])
    const imageUrl = useUnpaddedImageUrl(result?.image.paddedImageUrl?.url ?? result?.image.imageUrl?.url)
  if (!result) return null

  return <Container>
    <LargeImage src={imageUrl} />
    <MetaInfoContainer>
      <BatAdLabel onClick={e => e.stopPropagation()} href="brave://rewards">Ad</BatAdLabel>
      •
      {' ' + result.description}
    </MetaInfoContainer>
    <Title>
      <SecureLink href={result.targetUrl.url}>
        {result.title}
      </SecureLink>
    </Title>
    <CtaButton kind='outline'>{result.ctaText}</CtaButton>
  </Container>
}
