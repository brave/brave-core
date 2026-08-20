/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createPortal } from 'react-dom'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'

import { useAppState, useAppActions } from '../lib/app_context'

import { style } from './clear_ads_data_button.style'

const STATUS_DURATION_MS = 3000

export function ClearAdsDataButton() {
  const actions = useAppActions()
  const rewardsEnabled = useAppState((state) => state.rewardsEnabled)

  const [status, setStatus] = React.useState<'success' | 'error' | null>(null)

  React.useEffect(() => {
    if (!status) {
      return
    }
    const timeout = setTimeout(() => setStatus(null), STATUS_DURATION_MS)
    return () => clearTimeout(timeout)
  }, [status])

  async function clearAdsData() {
    const success = await actions.clearAdsData()
    setStatus(success ? 'success' : 'error')
  }

  if (rewardsEnabled) {
    return null
  }

  return (
    <>
      <Button
        size='small'
        onClick={clearAdsData}
      >
        Clear Ads Data
      </Button>
      {status &&
        createPortal(
          <Alert
            className='toast'
            data-css-scope={style.scope}
            type={status}
            role={status === 'success' ? 'status' : 'alert'}
            aria-live={status === 'success' ? 'polite' : 'assertive'}
          >
            {status === 'success'
              ? 'Ads data cleared.'
              : 'Failed to clear ads data.'}
          </Alert>,
          document.body,
        )}
    </>
  )
}
