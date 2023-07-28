// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'

import Dropdown from '@brave/leo/react/dropdown'
import {
  BraveNewsController
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import styled from 'styled-components'
import FeedPage from './FeedPage'
import SignalsPage from './SignalsPage'

const Container = styled.div`
  display: flex;
  flex-direction: column;
  gap: var(--leo-spacing-8);
  max-width: 800px;
  margin: 0 auto;
`

const Grid = styled.div`
  display: grid;
  grid-template-columns: 200px auto 200px;
  padding: 16px;
  gap: 8px;
`

export const api = BraveNewsController.getRemote();
const pages = ['feed', 'signals'] as const;
type Pages = (typeof pages)[number]

function App() {
  const [page, setPage] = React.useState<Pages>('signals')

  return (
    <Grid>
      <Dropdown value={page} onChange={e => setPage(e.detail.value)}>
        <span slot="label">Page</span>
        {pages.map(p => <leo-option key={p}>{p}</leo-option>)}
      </Dropdown>
      <Container>
        {page === 'feed' && <FeedPage />}
        {page === 'signals' && <SignalsPage />}
      </Container>
    </Grid>
  )
}

render(<App />, document.getElementById('root'))
