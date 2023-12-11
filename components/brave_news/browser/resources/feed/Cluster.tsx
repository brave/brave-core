// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import { Cluster as Info, ClusterType } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import Card from './Card';
import Article from './Article';
import styled from 'styled-components';
import { spacing } from '@brave/leo/tokens/css';
import { channelIcons } from '../shared/Icons';
import { MetaInfoContainer } from './ArticleMetaRow';
import { getTranslatedChannelName } from '../shared/channel';

interface Props {
  info: Info
}

const Container = styled(Card)`
  display: flex;
  flex-direction: column;
  gap: ${spacing['2Xl']};
  padding-top: ${spacing['2Xl']};
`

export default function Cluster({ info }: Props) {
  const groupName = info.type === ClusterType.CHANNEL
    ? getTranslatedChannelName(info.id)
    : info.id
  return <Container>
    <MetaInfoContainer>
      {channelIcons[info.id] ?? channelIcons.default} {groupName}
    </MetaInfoContainer>
    {info.articles.map((a, i) => {
      const info: any = a.article || a.hero
      return <Article key={i} info={info} hideChannel={info.type === ClusterType.CHANNEL} />
    })}
  </Container>
}
