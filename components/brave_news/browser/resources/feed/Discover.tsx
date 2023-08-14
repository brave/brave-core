// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import { Discover as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import styled from 'styled-components';
import Card from './Card';
import { useInspectContext } from '../context';

const Row = styled.div`
  display: flex;
  flex-direction: row;
  gap: 8px;
  justify-content: space-between;
`

interface Props {
  info: Info
}

export default function Component({ info }: Props) {
  const { publishers } = useInspectContext();
  return <Row>
    {info.publisherIds.map(p => <Card key={p}>
      {publishers[p]?.publisherName}
    </Card>)}
  </Row>
}
