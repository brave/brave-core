// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Dialog from '@brave/leo/react/dialog'
import { color } from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'
import { useBraveNews } from '../shared/Context'

import Loading from './Loading'

const Configure = React.lazy(() => import('./Configure'))

const StyledDialog = styled(Dialog)`
  --leo-dialog-width: 860px;
  --leo-dialog-padding: 0;
  --leo-dialog-background: ${color.container.background};
`

export default function BraveNewsModal() {
  const { customizePage, setCustomizePage } = useBraveNews()
  const shouldRender = !!customizePage

  if (!shouldRender) {
    return null
  }

  return (
    <StyledDialog
      isOpen
      showClose
      backdropClickCloses={false}
      onClose={() => setCustomizePage(null)}
    >
      <React.Suspense fallback={<Loading fill />}>
        <Configure />
      </React.Suspense>
    </StyledDialog>
  )
}
