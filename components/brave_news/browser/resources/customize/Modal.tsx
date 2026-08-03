/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Dialog from '@brave/leo/react/dialog'
import ProgressRing from '@brave/leo/react/progressRing'
import { useBraveNews } from '../shared/Context'

import { style } from './Modal.style'

const Configure = React.lazy(() => import('./Configure'))

export default function BraveNewsModal() {
  const { customizePage, setCustomizePage } = useBraveNews()
  const shouldRender = !!customizePage

  return shouldRender ? (
    <div data-css-scope={style.scope}>
      <Dialog
        isOpen={shouldRender}
        onClose={() => setCustomizePage(null)}
        backdropClickCloses
      >
        <React.Suspense
          fallback={
            <div className='loading'>
              <ProgressRing />
            </div>
          }
        >
          <Configure />
        </React.Suspense>
      </Dialog>
    </div>
  ) : null
}
