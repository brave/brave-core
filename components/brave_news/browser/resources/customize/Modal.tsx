// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { color, font } from '@brave/leo/tokens/css/variables'
import ProgressRing from '@brave/leo/react/progressRing'
import styled from 'styled-components'
import { useBraveNews } from '../shared/Context'

const Configure = React.lazy(() => import('./Configure'))

const Dialog = styled.dialog`
  font: ${font.default.regular};
  border-radius: 8px;
  border: none;
  margin: auto;
  width: min(100vw, 1049px);
  height: min(100vh, 712px);
  z-index: 1000;
  background: white;
  overflow: hidden;
  padding: 0;
  background-color: ${color.container.background};
  color:  ${color.text.primary};
`

const Loading = styled.div`
  --leo-progressring-size: 50px;

  display: flex;
  align-items: center;
  justify-content: center;
  height: 100%;
`

export default function BraveNewsModal() {
  const { customizePage, setCustomizePage } = useBraveNews()
  const dialogRef = React.useRef<HTMLDialogElement & { showModal: () => void, close: () => void, open: boolean }>()

  const shouldRender = !!customizePage

  // Note: There's no attribute for open modal, so we need
  // to call showModal instead.
  React.useEffect(() => {
    if (shouldRender && !dialogRef.current?.open) {
      dialogRef.current?.showModal?.()
    } else if (!shouldRender && dialogRef.current?.open) {
      dialogRef.current?.close?.()
    }

    const handleCancel = () => setCustomizePage(null)
    dialogRef.current?.addEventListener('cancel', handleCancel)
    return () => {
      dialogRef.current?.removeEventListener('cancel', handleCancel)
    }
  }, [shouldRender, dialogRef])

  // Only render the dialog if it should be shown, since
  // it is a complex view.
  return shouldRender ? <Dialog ref={dialogRef as any}>
    <React.Suspense fallback={<Loading><ProgressRing /></Loading>}>
      <Configure />
    </React.Suspense>
  </Dialog> : null
}
