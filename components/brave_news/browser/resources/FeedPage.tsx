// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { spacing } from '@brave/leo/tokens/css';
import * as React from 'react';
import styled from 'styled-components';
import Feed from './Feed';
import { useInspectContext } from './context';

const Container = styled.div`
  display: flex;
  flex-direction: column;
  gap: ${spacing.m};
  max-width: 560px;
  margin: 0 auto;
`

export default function FeedPage() {
  const { feed, truncate } = useInspectContext();
  return <Container>
    <h2>The Feed ({feed?.items.length} items. Truncated at {truncate})</h2>
    <Feed feed={feed} />
  </Container>
}
