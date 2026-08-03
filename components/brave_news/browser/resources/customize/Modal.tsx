/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Dialog from '@brave/leo/react/dialog'
import { useBraveNews } from '../shared/Context'

import { isCustomizeDialogCloseSuppressed } from './dialogCloseGuard'
import Loading from './Loading'
import { style } from './Modal.style'

const Configure = React.lazy(() => import('./Configure'))

export default function BraveNewsModal() {
  const { customizePage, setCustomizePage } = useBraveNews()
  const shouldRender = !!customizePage
  // Bumped to remount Leo Dialog when a suppressed close already tore it down
  // (e.g. native file picker firing `cancel` on the <dialog>).
  const [dialogKey, setDialogKey] = React.useState(0)

  if (!shouldRender) {
    return null
  }

  return (
    <div data-css-scope={style.scope}>
      <Dialog
        key={dialogKey}
        isOpen
        showClose
        // Programmatic OPML export clicks and the native file picker synthesize
        // events Leo would otherwise treat as an outside/escape close.
        backdropClickCloses={false}
        escapeCloses={false}
        onClose={() => {
          if (isCustomizeDialogCloseSuppressed()) {
            setDialogKey((key) => key + 1)
            return
          }
          setCustomizePage(null)
        }}
      >
        <React.Suspense fallback={<Loading fill />}>
          <Configure />
        </React.Suspense>
      </Dialog>
    </div>
  )
}
