/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Toggle from '@brave/leo/react/toggle'

import { useAppState, useAppActions } from '../lib/app_context'
import { ClearAdsDataButton } from './clear_ads_data_button'

// Every tab shares this same header shape (title, description, Auto-refresh,
// Refresh, Clear Ads Data); tab-specific controls (e.g. Logs' Errors
// only/Verbose mode) belong in a second `content-card` below, not here.
export function TabHeader(
  { title, description, onRefresh, headerActions, children }: {
    title: React.ReactNode
    description: React.ReactNode
    onRefresh: () => void
    // Tab-specific actions that belong in the top-right row alongside
    // Auto-refresh/Refresh (e.g. Events' Download), as opposed to `children`
    // below, which is for a whole extra row of controls.
    headerActions?: React.ReactNode
    children?: React.ReactNode
  },
) {
  const actions = useAppActions()
  const autoRefreshEnabled = useAppState((state) => state.autoRefreshEnabled)

  return (
    <div className='content-card'>
      <h4>
        <span className='title'>{title}</span>
        <Toggle
          size='small'
          checked={autoRefreshEnabled}
          onChange={() => actions.setAutoRefreshEnabled(!autoRefreshEnabled)}
        >
          Auto-refresh
        </Toggle>
        <Button
          size='small'
          onClick={onRefresh}
        >
          Refresh
        </Button>
        <ClearAdsDataButton />
        {headerActions}
      </h4>
      <p>{description}</p>
      {children}
    </div>
  )
}
