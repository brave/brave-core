// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import Card, { Title } from './Card';
import Button from '@brave/leo/react/button';
import { useBraveNews } from '../shared/Context';
import { getLocale } from '$web-common/locale';
import styled from 'styled-components';
import { spacing } from '@brave/leo/tokens/css';

const Container = styled(Card)`
  display: flex;
  align-items: center;
  gap: ${spacing.m};

  & > leo-button {
    flex: 0;
  }
`

export default function NoArticles() {
  const { refreshFeedV2 } = useBraveNews()
  return <Container>
    <Title>{getLocale('braveNewsNoArticles')}</Title>
    <Button onClick={refreshFeedV2}>{getLocale('braveNewsRefreshFeed')}</Button>
  </Container>
}
