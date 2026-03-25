// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// brave://wallet/dev-polkadot — Polkadot snap dev/test page.
// Renders the snap's onHomePage UI via SnapUIRenderer.

import * as React from 'react'
import getWalletPageApiProxy from '../../../page/wallet_page_api_proxy'
import {
  SnapHomePageRenderer,
  type SnapUserInputEvent,
} from '../../../common/snap/snap_ui_renderer'

const POLKADOT_SNAP_ID = 'npm:@polkagate/snap'

const btnStyle = (disabled?: boolean): React.CSSProperties => ({
  padding: '8px 16px',
  borderRadius: '6px',
  border: '1px solid #0c5460',
  backgroundColor: '#17a2b8',
  color: 'white',
  cursor: disabled ? 'not-allowed' : 'pointer',
  fontSize: '13px',
  opacity: disabled ? 0.6 : 1,
  marginRight: '8px',
})

export function DevPolkadot() {
  const [homePageData, setHomePageData] = React.useState<object | null>(null)
  const [interfaceId, setInterfaceId] = React.useState<string | null>(null)
  const [loading, setLoading] = React.useState(false)
  const [error, setError] = React.useState<string | null>(null)

  const handleLoadHomePage = React.useCallback(async () => {
    setLoading(true)
    setError(null)
    try {
      const { snapBridge } = getWalletPageApiProxy()
      const result = await snapBridge.getHomePage(POLKADOT_SNAP_ID)
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
  }, [])

  const handleUserInput = React.useCallback(
    async (event: SnapUserInputEvent) => {
      if (!interfaceId) {
        return
      }
      const { snapBridge } = getWalletPageApiProxy()
      const result = await snapBridge.sendUserInput(
        POLKADOT_SNAP_ID,
        interfaceId,
        event,
      )
      if (result.error) {
        setError(result.error)
        return
      }
      if (result.content) {
        setHomePageData(result.content as object)
      }
    },
    [interfaceId],
  )

  const handleUnload = () => {
    getWalletPageApiProxy().snapBridge.unloadSnap(POLKADOT_SNAP_ID)
    setHomePageData(null)
    setInterfaceId(null)
    setError(null)
  }

  return (
    <div style={{ padding: '24px', maxWidth: '720px', fontFamily: 'sans-serif' }}>
      <h2 style={{ fontFamily: 'monospace', fontSize: '18px', marginBottom: '4px' }}>
        Polkadot Snap
      </h2>
      <p style={{ color: '#666', fontSize: '13px', margin: '0 0 20px' }}>
        Snap ID: <code>{POLKADOT_SNAP_ID}</code>
      </p>

      <div style={{ marginBottom: '16px', display: 'flex', gap: '8px' }}>
        <button
          onClick={handleLoadHomePage}
          disabled={loading}
          style={btnStyle(loading)}
        >
          {loading ? 'Loading…' : 'Load Homepage'}
        </button>
        <button
          onClick={handleUnload}
          style={{
            ...btnStyle(),
            backgroundColor: '#dc3545',
            border: '1px solid #dc3545',
          }}
        >
          Unload Snap
        </button>
      </div>

      {error && (
        <div
          style={{
            color: '#721c24',
            backgroundColor: '#f8d7da',
            border: '1px solid #f5c6cb',
            borderRadius: '6px',
            padding: '8px 12px',
            marginBottom: '16px',
            fontSize: '13px',
          }}
        >
          {error}
        </div>
      )}

      {homePageData && (
        <div
          style={{
            border: '1px solid #ddd',
            borderRadius: '8px',
            padding: '16px',
          }}
        >
          <SnapHomePageRenderer
            data={homePageData}
            onUserInput={handleUserInput}
          />
        </div>
      )}
    </div>
  )
}

export default DevPolkadot
