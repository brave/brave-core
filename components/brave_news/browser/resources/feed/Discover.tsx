// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { Discover as Info, Publisher } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import * as React from 'react';
import styled from 'styled-components';
import { publishersCache, useBraveNews } from '../shared/Context';
import Card, { Title } from './Card';
import { isPublisherEnabled } from '../shared/api';
import Button from '@brave/leo/react/button';
import Icon from '@brave/leo/react/icon';
import { useLazyUnpaddedImageUrl } from '../shared/useUnpaddedImageUrl';
import { font } from '@brave/leo/tokens/css';

const Row = styled.div`
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 8px;
  margin-top: 8px;
`

const SuggestionCard = styled(Card) <{ backgroundUrl?: string, backgroundColor?: string }>`
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;

  height: 80px;

  background-image: ${p => `url(${p.backgroundUrl})`}, ${p => `linear-gradient(${p.backgroundColor}, ${p.backgroundColor})`};
  background-size: contain;
  background-repeat: no-repeat;
  background-position: center;
`

const FollowButton = styled(Button)`
  --leo-button-padding: 0;

  position: absolute;
  top: 12px;
  right: 12px;
`

const FollowButtonIcon = styled(Icon)`
  --leo-icon-size: 24px;
`

const PublisherTitle = styled.span`
  font: ${font.primary.default.regular};
`

interface Props {
  info: Info
}

function Suggestion({ publisher }: { publisher: Publisher }) {
  const { setElementRef, url: coverUrl } = useLazyUnpaddedImageUrl(publisher.coverUrl?.url, { useCache: true })
  return <div>
    <SuggestionCard backgroundUrl={coverUrl} backgroundColor={publisher.backgroundColor} ref={setElementRef} title={publisher.publisherName}>
      {!isPublisherEnabled(publisher) && <FollowButton size='small' kind='plain-faint' fab onClick={() => publishersCache.setPublisherFollowed(publisher.publisherId, true)}>
        <FollowButtonIcon name="plus-add-circle" />
      </FollowButton>}
    </SuggestionCard>
    <PublisherTitle>{publisher?.publisherName}</PublisherTitle>
  </div>
}

export default function Component({ info }: Props) {
  const { publishers } = useBraveNews();
  return <Card>
    <Title><Icon name="star-outline" /> Sources you'll enjoy</Title>
    <Row>
      {info.publisherIds.map(p => <Suggestion key={p} publisher={publishers[p]} />)}
    </Row>
  </Card>
}
