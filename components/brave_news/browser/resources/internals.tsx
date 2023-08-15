// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'

import styled from 'styled-components'
import InspectContext, { useInspectContext } from './context'
import PageInfo from './PageInfo'
import FeedPage from './FeedPage'
import SignalsPage from './SignalsPage'

const Grid = styled.div`
  display: grid;
  grid-template-columns: 200px auto 200px;
  padding: 16px;
  gap: 8px;
`

const Container = styled.div`
  display: flex;
  flex-direction: column;
  gap: var(--leo-spacing-8);
  max-width: 800px;
  margin: 0 auto;
`

function App() {
  const { page, publishers, channels } = useInspectContext()

  return (
    <Grid>
      <PageInfo />
      <Container>
        {page === 'feed' && <FeedPage />}
        {page === 'signals' && <SignalsPage />}
      </Container>
      <div>
        Publishers: {Object.keys(publishers).length}
        <br />
        Channels: {Object.keys(channels).length}
      </div>
    </Grid>
  )
}

render(<InspectContext>
  <App />
</InspectContext>, document.getElementById('root'))
