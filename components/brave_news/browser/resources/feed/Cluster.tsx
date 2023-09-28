// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import { Cluster as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import Card from './Card';
import Article from './Article';
import styled from 'styled-components';
import { spacing } from '@brave/leo/tokens/css';
import { channelIcons } from '../../../../brave_new_tab_ui/components/default/braveNews/customize/Icons';
import { MetaInfoContainer } from './ArticleMetaRow';

interface Props {
  info: Info
}

const Container = styled(Card)`
  display: flex;
  flex-direction: column;
  gap: ${spacing.m};
`

export default function Cluster({ info }: Props) {
  return <Container>
    <MetaInfoContainer>
      {channelIcons[info.id] ?? channelIcons.default} {info.id}
    </MetaInfoContainer>
    {info.articles.map((a, i) => <Article key={i} info={a.article || a.hero as any} hideChannel />)}
  </Container>
}
