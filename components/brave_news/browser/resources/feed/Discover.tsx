// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { getLocale } from '$web-common/locale';
import Icon from '@brave/leo/react/icon';
import { icon, spacing } from '@brave/leo/tokens/css/variables';
import { Discover as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import * as React from 'react';
import styled from 'styled-components';
import PublisherCard from '../shared/PublisherCard';
import Card, { Title } from './Card';

const Row = styled.div`
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: ${spacing['2Xl']};
  margin-top: ${spacing.m};
`

const TitleIcon = styled(Icon)`
  --leo-icon-size: ${icon.s};
  margin-right: ${spacing.s};
`

const Container = styled(Card)`
  padding: ${spacing['2Xl']} ${spacing.xl};
`

interface Props {
  info: Info
}

export default function Component({ info }: Props) {
  return <Container>
    <Title>
      <TitleIcon name="star-outline" />
      {getLocale('braveNewsSourcesRecommendation')}
    </Title>
    <Row>
      {info.publisherIds.map(p => <PublisherCard key={p} publisherId={p} />)}
    </Row>
  </Container>
}
