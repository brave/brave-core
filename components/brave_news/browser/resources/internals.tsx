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
import FeedNavigation from './FeedNavigation'
import Variables from './Variables'

setIconBasePath('//resources/brave-icons')

const Grid = styled(Variables)`
  display: grid;
  grid-template-columns: 300px auto 300px;
  padding: 16px;
  gap: 8px;

  backdrop-filter: blur(64px);
  background: rgba(255, 255, 255, 0.7);

  min-height: 100vh;
`

function App() {
  return <Grid data-theme="dark">
    <SignalsPage />
    <FeedPage />
    <FeedNavigation />
  </Grid>
}

render(<InspectContext>
  <App />
</InspectContext>, document.getElementById('root'))
