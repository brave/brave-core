// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'

import styled from 'styled-components'
import InspectContext from './context'
import PageInfo from './PageInfo'
import CurrentPage from './CurrentPage'

const Grid = styled.div`
  display: grid;
  grid-template-columns: 200px auto 200px;
  padding: 16px;
  gap: 8px;
`


function App() {
  return (
    <InspectContext>
      <Grid>
        <PageInfo />
        <CurrentPage />
        <div />
      </Grid>
    </InspectContext>
  )
}

render(<App />, document.getElementById('root'))
