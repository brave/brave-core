// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import Card from './Card';
import { Article as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import styled from 'styled-components';
import { color } from '@brave/leo/tokens/css';

interface Props {
  info: Info
}

const Publisher = styled.div`
  color: ${color.text.secondary};
`

const Description = styled.div`
  max-height: 100px;
  overflow: hidden;
`

export default function Article({ info }: Props) {
  return <Card onClick={() => window.open(info.data.url.url, '_blank', 'noopener noreferrer')}>
    <h4>{info.data.title}{info.isDiscover && " (discovering)"}</h4>
    <Publisher>{info.data.publisherName} - {info.data.relativeTimeDescription}</Publisher>
    <Description>{info.data.description}</Description>
  </Card>
}
