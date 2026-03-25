// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// brave://wallet/snap/:snapId — renders the home page UI for any installed snap.

import * as React from 'react'
import { useParams, useHistory } from 'react-router-dom'

import { WalletRoutes } from '../../../constants/types'
import getWalletPageApiProxy from '../../wallet_page_api_proxy'
import {
  SnapHomePageRenderer,
  type SnapUserInputEvent,
} from '../../../common/snap/snap_ui_renderer'

export function SnapPage() {
  const { snapId: encodedSnapId } = useParams<{ snapId: string }>()
  const snapId = decodeURIComponent(encodedSnapId)
  const history = useHistory()

  const [homePageData, setHomePageData] = React.useState<object | null>(null)
  const [interfaceId, setInterfaceId] = React.useState<string | null>(null)
  const [loading, setLoading] = React.useState(false)
  const [error, setError] = React.useState<string | null>(null)

  const loadHomePage = React.useCallback(async () => {
    setLoading(true)
    setError(null)
    try {
      const { snapBridge } = getWalletPageApiProxy()
      const result = await snapBridge.getHomePage(snapId)
      if (result.error) {
        setError(result.error)
      } else {
        setHomePageData(result.content as object | null)
        setInterfaceId(result.interfaceId ?? null)
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : String(err))
    } finally {
      setLoading(false)
    }
  }, [snapId])

  // Auto-load on mount.
  React.useEffect(() => {
    loadHomePage()
  }, [loadHomePage])

  const handleUserInput = React.useCallback(
    async (event: SnapUserInputEvent) => {
      if (!interfaceId) {
        return
      }
      const { snapBridge } = getWalletPageApiProxy()
      const result = await snapBridge.sendUserInput(snapId, interfaceId, event)
      if (result.error) {
        setError(result.error)
        return
      }
      if (result.content) {
        setHomePageData(result.content as object)
      }
    },
    [snapId, interfaceId],
  )

  const handleUnload = React.useCallback(() => {
    getWalletPageApiProxy().snapBridge.unloadSnap(snapId)
    setHomePageData(null)
    setInterfaceId(null)
    setError(null)
  }, [snapId])

  return (
    <div style={styles.page}>
      {/* Header */}
      <div style={styles.header}>
        <button
          style={styles.backBtn}
          onClick={() => history.push(WalletRoutes.SnapsStore)}
        >
          ← Snaps Store
        </button>
        <div style={styles.titleRow}>
          <h2 style={styles.heading}>{snapId}</h2>
          <div style={styles.headerActions}>
            <button
              style={styles.reloadBtn}
              onClick={loadHomePage}
              disabled={loading}
            >
              {loading ? 'Loading…' : 'Reload'}
            </button>
            <button
              style={styles.unloadBtn}
              onClick={handleUnload}
            >
              Unload
            </button>
          </div>
        </div>
      </div>

      {/* Error */}
      {error && (
        <div style={styles.errorBox}>
          {error}
        </div>
      )}

      {/* Loading spinner */}
      {loading && !homePageData && (
        <div style={styles.loadingBox}>Loading snap UI…</div>
      )}

      {/* Snap UI */}
      {homePageData && (
        <div style={styles.contentBox}>
          <SnapHomePageRenderer
            data={homePageData}
            onUserInput={handleUserInput}
          />
        </div>
      )}

      {/* Empty state */}
      {!loading && !homePageData && !error && (
        <div style={styles.emptyBox}>
          <p>This snap does not provide a home page UI.</p>
        </div>
      )}
    </div>
  )
}

const styles: Record<string, React.CSSProperties> = {
  page: {
    padding: '24px',
    maxWidth: '800px',
    fontFamily: 'system-ui, sans-serif',
    color: '#1d1f25',
  },
  header: {
    marginBottom: '20px',
  },
  backBtn: {
    background: 'none',
    border: 'none',
    color: '#6b7280',
    cursor: 'pointer',
    fontSize: '13px',
    padding: '0 0 8px',
  },
  titleRow: {
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'space-between',
    gap: '12px',
  },
  heading: {
    margin: 0,
    fontSize: '16px',
    fontWeight: 600,
    fontFamily: 'monospace',
    wordBreak: 'break-all' as const,
  },
  headerActions: {
    display: 'flex',
    gap: '8px',
    flexShrink: 0,
  },
  reloadBtn: {
    padding: '6px 14px',
    fontSize: '13px',
    background: '#17a2b8',
    color: '#fff',
    border: 'none',
    borderRadius: '6px',
    cursor: 'pointer',
  },
  unloadBtn: {
    padding: '6px 14px',
    fontSize: '13px',
    background: 'transparent',
    color: '#d32f2f',
    border: '1px solid #d32f2f',
    borderRadius: '6px',
    cursor: 'pointer',
  },
  errorBox: {
    background: '#fde8e8',
    border: '1px solid #f5c6cb',
    borderRadius: '6px',
    padding: '10px 14px',
    fontSize: '13px',
    color: '#721c24',
    marginBottom: '16px',
  },
  loadingBox: {
    padding: '32px',
    textAlign: 'center' as const,
    color: '#6b7280',
    fontSize: '14px',
  },
  contentBox: {
    border: '1px solid #e0e2e8',
    borderRadius: '8px',
    padding: '16px',
  },
  emptyBox: {
    padding: '32px',
    textAlign: 'center' as const,
    color: '#6b7280',
    fontSize: '14px',
  },
}

export default SnapPage
