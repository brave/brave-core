// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'

import Button from '@brave/leo/react/button'
import { setIconBasePath } from '@brave/leo/react/icon'
import styled from 'styled-components'
import { downloadExport, getExportData } from './export'
import Variables from './Variables'
import usePromise from '$web-common/usePromise'

setIconBasePath('//resources/brave-icons')

const Grid = styled(Variables)`
  display: grid;
  grid-template-columns: 300px auto 300px;
  padding: 16px;
  gap: 8px;

  min-height: 100vh;
`

function App() {
  const { result: exportData } = usePromise(getExportData, [])
  return <Grid data-theme="dark">
    <Button onClick={downloadExport}>
      Export
    </Button>
    <pre>
      {exportData ?? '...'}
    </pre>
  </Grid>
}

createRoot(document.getElementById('root')!)
  .render(<App />)
