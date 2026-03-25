// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'

import {
  useGetSnapHomePageQuery,
  useSendSnapUserInputMutation,
} from '../../../common/slices/api.slice'
import {
  SnapHomePageRenderer,
  type SnapUserInputEvent,
} from '../../../common/snap/snap_ui_renderer'
import { useAppDispatch } from '../../../common/hooks/use-redux'
import { PanelActions } from '../../../panel/actions'

interface Props {
  snapId: string
}

export const SnapViewPanel = ({ snapId }: Props) => {
  const dispatch = useAppDispatch()

  const {
    data,
    isLoading,
    error: queryError,
    refetch,
  } = useGetSnapHomePageQuery({ snapId })

  const [sendUserInput] = useSendSnapUserInputMutation()

  // Local override for updated content after user interactions.
  const [liveContentJson, setLiveContentJson] = React.useState<string | null>(null)
  const [liveInterfaceId, setLiveInterfaceId] = React.useState<string | null>(null)

  // Reset live state when snapId changes or fresh data arrives.
  React.useEffect(() => {
    setLiveContentJson(null)
    setLiveInterfaceId(null)
  }, [snapId, data])

  const contentJson = liveContentJson ?? data?.contentJson ?? null
  const interfaceId = liveInterfaceId ?? data?.interfaceId ?? null
  const fetchError = data?.error ?? (queryError ? String(queryError) : null)

  const content = React.useMemo(() => {
    if (!contentJson) return null
    try { return JSON.parse(contentJson) } catch { return null }
  }, [contentJson])

  const handleUserInput = React.useCallback(
    async (event: SnapUserInputEvent) => {
      if (!interfaceId) return
      const result = await sendUserInput({
        snapId,
        interfaceId,
        eventJson: JSON.stringify(event),
      }).unwrap()
      if (result.contentJson) {
        setLiveContentJson(result.contentJson)
        setLiveInterfaceId(interfaceId)
      }
    },
    [snapId, interfaceId, sendUserInput],
  )

  const handleBack = () => {
    dispatch(PanelActions.navigateTo('snaps'))
  }

  return (
    <div style={styles.page}>
      <div style={styles.header}>
        <button
          style={styles.backBtn}
          onClick={handleBack}
        >
          ← Snaps
        </button>
        <button
          style={styles.reloadBtn}
          onClick={refetch}
          disabled={isLoading}
        >
          ↺
        </button>
      </div>

      <div style={styles.snapId}>{snapId}</div>

      {isLoading && (
        <div style={styles.centered}>
          <ProgressRing mode='indeterminate' />
          <p style={styles.loadingText}>Loading snap…</p>
        </div>
      )}

      {!isLoading && fetchError && (
        <div style={styles.errorBox}>
          {fetchError === 'no_bridge'
            ? 'Open brave://wallet in a tab first to activate snap support.'
            : fetchError}
        </div>
      )}

      {!isLoading && !fetchError && content && (
        <div style={styles.content}>
          <SnapHomePageRenderer
            data={content}
            onUserInput={handleUserInput}
          />
        </div>
      )}

      {!isLoading && !fetchError && !content && (
        <div style={styles.centered}>
          <p style={styles.loadingText}>This snap has no home page.</p>
        </div>
      )}
    </div>
  )
}

const styles: Record<string, React.CSSProperties> = {
  page: {
    display: 'flex',
    flexDirection: 'column',
    height: '100%',
    fontFamily: 'system-ui, sans-serif',
    overflowY: 'auto',
  },
  header: {
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'space-between',
    padding: '10px 12px',
    borderBottom: '1px solid #e0e2e8',
  },
  backBtn: {
    background: 'none',
    border: 'none',
    color: '#6b7280',
    cursor: 'pointer',
    fontSize: '13px',
    padding: 0,
  },
  reloadBtn: {
    background: 'none',
    border: 'none',
    color: '#6b7280',
    cursor: 'pointer',
    fontSize: '16px',
    padding: '0 4px',
  },
  snapId: {
    padding: '8px 12px',
    fontSize: '11px',
    color: '#9ca3af',
    fontFamily: 'monospace',
    borderBottom: '1px solid #f0f1f4',
    overflow: 'hidden',
    textOverflow: 'ellipsis',
    whiteSpace: 'nowrap',
  },
  content: {
    flex: 1,
    padding: '12px',
    overflowY: 'auto',
  },
  centered: {
    flex: 1,
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    justifyContent: 'center',
    gap: '12px',
    padding: '24px',
  },
  loadingText: {
    margin: 0,
    fontSize: '13px',
    color: '#6b7280',
  },
  errorBox: {
    margin: '16px',
    padding: '12px',
    background: '#fde8e8',
    border: '1px solid #f5c6cb',
    borderRadius: '6px',
    fontSize: '13px',
    color: '#721c24',
  },
}

export default SnapViewPanel
