// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// brave://wallet/dev-filecoin — Filecoin snap dev/test page.
// Renders the snap's onHomePage UI via SnapUIRenderer.

import * as React from 'react'
import { getAPIProxy } from '../../../common/async/bridge'
import getWalletPageApiProxy from '../../../page/wallet_page_api_proxy'
import {
  SnapHomePageRenderer,
  type SnapUserInputEvent,
} from '../../../common/snap/snap_ui_renderer'

const FILECOIN_SNAP_ID = 'npm:filsnap'

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

async function callFilSnap<T = unknown>(
  method: string,
  params?: Record<string, unknown>,
): Promise<T> {
  const { braveWalletService } = getAPIProxy()
  const { resultJson, error } = await (braveWalletService as any).invokeSnap(
    FILECOIN_SNAP_ID,
    method,
    JSON.stringify(params ?? {}),
  )
  if (error) {
    throw new Error(error)
  }
  return JSON.parse(resultJson ?? 'null') as T
}

export function DevFilecoin() {
  const [homePageData, setHomePageData] = React.useState<object | null>(null)
  const [interfaceId, setInterfaceId] = React.useState<string | null>(null)
  const [loading, setLoading] = React.useState<string | null>(null)
  const [error, setError] = React.useState<string | null>(null)
  const [network, setNetwork] = React.useState<'mainnet' | 'testnet'>('mainnet')

  const withLoading = React.useCallback(
    async (key: string, fn: () => Promise<void>) => {
      setLoading(key)
      setError(null)
      try {
        await fn()
      } catch (err) {
        setError(err instanceof Error ? err.message : String(err))
      } finally {
        setLoading(null)
      }
    },
    [],
  )

  const handleConfigure = () =>
    withLoading('configure', async () => {
      await callFilSnap('fil_configure', {
        configuration: {
          network: network === 'mainnet' ? 'f' : 't',
          rpc: {
            url:
              network === 'mainnet'
                ? 'https://api.node.glif.io'
                : 'https://api.calibration.node.glif.io',
            token: '',
          },
        },
      })
    })

  const handleLoadHomePage = () =>
    withLoading('homepage', async () => {
      const { snapBridge } = getWalletPageApiProxy()
      const result = await snapBridge.getHomePage(FILECOIN_SNAP_ID)
      if (result.error) {
        throw new Error(result.error)
      }
      setHomePageData(result.content as object | null)
      setInterfaceId(result.interfaceId ?? null)
    })

  const handleUserInput = React.useCallback(
    async (event: SnapUserInputEvent) => {
      if (!interfaceId) {
        return
      }
      const { snapBridge } = getWalletPageApiProxy()
      const result = await snapBridge.sendUserInput(
        FILECOIN_SNAP_ID,
        interfaceId,
        event,
      )
      if (result.error) {
        setError(result.error)
        return
      }
      // The snap may have called snap_updateInterface — re-render with new content.
      if (result.content) {
        setHomePageData(result.content as object)
      }
    },
    [interfaceId],
  )

  const handleUnload = () => {
    getWalletPageApiProxy().snapBridge.unloadSnap(FILECOIN_SNAP_ID)
    setHomePageData(null)
    setInterfaceId(null)
    setError(null)
  }

  return (
    <div style={{ padding: '24px', maxWidth: '720px', fontFamily: 'sans-serif' }}>
      <h2 style={{ fontFamily: 'monospace', fontSize: '18px', marginBottom: '4px' }}>
        Filecoin Snap
      </h2>
      <p style={{ color: '#666', fontSize: '13px', margin: '0 0 20px' }}>
        Snap ID: <code>{FILECOIN_SNAP_ID}</code>
      </p>

      {/* Step 1 — Configure */}
      <div
        style={{
          border: '1px solid #ddd',
          borderRadius: '8px',
          padding: '16px',
          marginBottom: '12px',
        }}
      >
        <h3 style={{ fontSize: '14px', margin: '0 0 12px', fontWeight: 600 }}>
          1. Configure network
        </h3>
        <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
          <select
            value={network}
            onChange={(e) => setNetwork(e.target.value as 'mainnet' | 'testnet')}
            style={{
              padding: '6px 10px',
              borderRadius: '6px',
              border: '1px solid #ccc',
              fontSize: '13px',
            }}
          >
            <option value='mainnet'>Mainnet (f-addresses, coinType 461)</option>
            <option value='testnet'>Testnet / Calibration (t-addresses, coinType 1)</option>
          </select>
          <button
            onClick={handleConfigure}
            disabled={loading === 'configure'}
            style={btnStyle(loading === 'configure')}
          >
            {loading === 'configure' ? 'Configuring…' : 'Configure'}
          </button>
        </div>
      </div>

      {/* Step 2 — Load homepage */}
      <div
        style={{
          border: '1px solid #ddd',
          borderRadius: '8px',
          padding: '16px',
          marginBottom: '12px',
        }}
      >
        <h3 style={{ fontSize: '14px', margin: '0 0 12px', fontWeight: 600 }}>
          2. Snap homepage
        </h3>
        <button
          onClick={handleLoadHomePage}
          disabled={loading === 'homepage'}
          style={btnStyle(loading === 'homepage')}
        >
          {loading === 'homepage' ? 'Loading…' : 'Load Homepage'}
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

export default DevFilecoin
