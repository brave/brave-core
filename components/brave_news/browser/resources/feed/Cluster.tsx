// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import { Cluster as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import Card, { MetaInfo } from './Card';
import Article from './Article';
import IconReact from '@brave/leo/react/icon';
import styled from 'styled-components';
import { spacing } from '@brave/leo/tokens/css';

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
    <MetaInfo>
      <IconReact name="fire" />
      {info.type}: {info.id}
    </MetaInfo>
    {info.articles.map((a, i) => <Article key={i} info={a.article || a.hero as any}/>)}
  </Container>
}
