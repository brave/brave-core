// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'

import { setIconBasePath } from '@brave/leo/react/icon'
import styled from 'styled-components'
import FeedPage from './FeedPage'
import SignalsPage from './SignalsPage'
import InspectContext from './context'

setIconBasePath('//resources/brave-icons')

const Grid = styled.div`
  display: grid;
  grid-template-columns: 400px auto;
  padding: 16px;
  gap: 8px;
`

const Blur = styled.div`
  --edges: -128px;

  z-index: -1;
  position: fixed;
  top: var(--edges); left: var(--edges); bottom: var(--edges); right: var(--edges);
  filter: blur(64px);
  background: rgba(255, 255, 255, 0.7);
`

function App() {
  return <>
    <Blur />
    <Grid>
      <SignalsPage />
      <FeedPage />
    </Grid>
  </>
}

render(<InspectContext>
  <App />
</InspectContext>, document.getElementById('root'))
