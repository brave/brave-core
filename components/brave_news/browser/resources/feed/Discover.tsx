// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { Discover as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import * as React from 'react';
import styled from 'styled-components';
import { useBraveNews } from '../shared/Context';
import Card from './Card';

const Row = styled.div`
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 8px;
  margin-top: 8px;
`

const Suggestion = styled(Card)`
  display: flex;
  align-items: center;
  justify-content: center;
`

interface Props {
  info: Info
}

export default function Component({ info }: Props) {
  const { publishers } = useBraveNews();
  return <Card>
    Based on your interests, you might like these publishers:
    <Row>
      {info.publisherIds.map(p => <Suggestion key={p}>
        {publishers[p]?.publisherName}
      </Suggestion>)}
    </Row>
  </Card>
}
