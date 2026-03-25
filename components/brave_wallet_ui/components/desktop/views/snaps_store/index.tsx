// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router-dom'

import {
  useGetInstalledSnapsQuery,
  useUninstallSnapMutation,
  useRequestInstallSnapMutation,
  useGetPendingSnapInstallQuery,
} from '../../../../common/slices/api.slice'
import { BraveWallet } from '../../../../constants/types'
import getWalletPageApiProxy from '../../../../page/wallet_page_api_proxy'

export const SnapsStore = () => {
  const history = useHistory()
  const [snapIdInput, setSnapIdInput] = React.useState('')
  const [statusMessage, setStatusMessage] = React.useState('')
  const [isError, setIsError] = React.useState(false)

  const { data: snaps = [] as BraveWallet.InstalledSnapInfo[], refetch: refetchSnaps } = useGetInstalledSnapsQuery()
  const { data: pendingSnap } = useGetPendingSnapInstallQuery()
  const [requestInstallSnap, { isLoading: isRequesting }] =
    useRequestInstallSnapMutation()
  const [uninstallSnap] = useUninstallSnapMutation()

  // Refresh installed list when a pending install resolves to idle.
  const prevStateRef = React.useRef<BraveWallet.SnapInstallState | undefined>()
  React.useEffect(() => {
    const prev = prevStateRef.current
    const cur = pendingSnap?.state
    if (
      prev !== undefined
      && prev !== BraveWallet.SnapInstallState.kIdle
      && cur === BraveWallet.SnapInstallState.kIdle
    ) {
      refetchSnaps()
      setStatusMessage('Snap installed successfully.')
      setIsError(false)
    }
    prevStateRef.current = cur
  }, [pendingSnap?.state, refetchSnaps])

  const handleInstall = React.useCallback(async () => {
    const id = snapIdInput.trim()
    if (!id) {
      setStatusMessage('Please enter a snap ID (e.g. npm:@polkagate/snap)')
      setIsError(true)
      return
    }

    setStatusMessage('')
    setIsError(false)

    try {
      const result = await requestInstallSnap({
        snapId: id,
        version: 'latest',
      }).unwrap()

      if (!result.success) {
        if (result.error === 'already_pending') {
          setStatusMessage(
            'Another snap install is pending — approve or reject it in the wallet panel first.',
          )
        } else {
          setStatusMessage(`Error: ${result.error ?? 'unknown error'}`)
        }
        setIsError(true)
        return
      }

      // Install started — open the panel for approval.
      setStatusMessage('Waiting for approval in wallet panel…')
      setIsError(false)
      setSnapIdInput('')
      getWalletPageApiProxy().pageHandler?.showApprovePanelUI()
    } catch (err) {
      setStatusMessage(`Error: ${String(err)}`)
      setIsError(true)
    }
  }, [snapIdInput, requestInstallSnap])

  const handleUninstall = React.useCallback(
    async (snapId: string) => {
      await uninstallSnap({ snapId })
      setStatusMessage(`Uninstalled ${snapId}`)
      setIsError(false)
    },
    [uninstallSnap],
  )

  const handleKeyDown = React.useCallback(
    (e: React.KeyboardEvent<HTMLInputElement>) => {
      if (e.key === 'Enter') {
        handleInstall()
      }
    },
    [handleInstall],
  )

  const isPendingActive =
    pendingSnap && pendingSnap.state !== BraveWallet.SnapInstallState.kIdle

  return (
    <div style={styles.page}>
      <h2 style={styles.heading}>Snaps Store</h2>

      {/* Install form */}
      <div style={styles.installBox}>
        <h3 style={styles.sectionTitle}>Install a Snap</h3>
        <div style={styles.inputRow}>
          <input
            style={styles.input}
            type='text'
            placeholder='npm:@polkagate/snap'
            value={snapIdInput}
            onChange={(e) => setSnapIdInput(e.target.value)}
            onKeyDown={handleKeyDown}
            disabled={isRequesting || !!isPendingActive}
          />
          <button
            style={styles.button}
            onClick={handleInstall}
            disabled={isRequesting || !snapIdInput.trim() || !!isPendingActive}
          >
            {isRequesting ? 'Starting…' : 'Install'}
          </button>
        </div>
        {isPendingActive && (
          <p style={styles.pendingText}>
            Install in progress — check the wallet panel.
          </p>
        )}
        {statusMessage && (
          <p style={isError ? styles.errorText : styles.successText}>
            {statusMessage}
          </p>
        )}
      </div>

      {/* Installed snaps list */}
      <div style={styles.listBox}>
        <h3 style={styles.sectionTitle}>
          Installed Snaps ({snaps.length})
        </h3>
        {snaps.length === 0 ? (
          <p style={styles.emptyText}>No snaps installed yet.</p>
        ) : (
          <table style={styles.table}>
            <thead>
              <tr>
                <th style={styles.th}>Snap ID</th>
                <th style={styles.th}>Name</th>
                <th style={styles.th}>Version</th>
                <th style={styles.th}>Permissions</th>
                <th style={styles.th}></th>
              </tr>
            </thead>
            <tbody>
              {snaps.map((snap) => (
                <tr
                  key={snap.snapId}
                  style={styles.tr}
                >
                  <td style={styles.td}>
                    <code style={styles.code}>{snap.snapId}</code>
                  </td>
                  <td style={styles.td}>{snap.proposedName || '—'}</td>
                  <td style={styles.td}>{snap.version}</td>
                  <td style={styles.td}>
                    <span style={styles.permList}>
                      {snap.permissions.join(', ') || '—'}
                    </span>
                  </td>
                  <td style={styles.td}>
                    <div style={styles.actionButtons}>
                      <button
                        style={styles.openButton}
                        onClick={() =>
                          history.push(
                            `/snap/${encodeURIComponent(snap.snapId)}`,
                          )
                        }
                      >
                        Open
                      </button>
                      <button
                        style={styles.uninstallButton}
                        onClick={() => handleUninstall(snap.snapId)}
                      >
                        Uninstall
                      </button>
                    </div>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>
    </div>
  )
}

const styles: Record<string, React.CSSProperties> = {
  page: {
    padding: '24px',
    maxWidth: '900px',
    fontFamily: 'system-ui, sans-serif',
    color: '#1d1f25',
  },
  heading: {
    margin: '0 0 20px',
    fontSize: '22px',
    fontWeight: 600,
  },
  sectionTitle: {
    margin: '0 0 12px',
    fontSize: '15px',
    fontWeight: 600,
  },
  installBox: {
    background: '#f8f9fa',
    border: '1px solid #e0e2e8',
    borderRadius: '8px',
    padding: '16px',
    marginBottom: '24px',
  },
  inputRow: {
    display: 'flex',
    gap: '8px',
    alignItems: 'center',
  },
  input: {
    flex: 1,
    padding: '8px 12px',
    fontSize: '14px',
    border: '1px solid #c0c4d0',
    borderRadius: '6px',
    outline: 'none',
  },
  button: {
    padding: '8px 20px',
    fontSize: '14px',
    fontWeight: 500,
    background: '#fb542b',
    color: '#fff',
    border: 'none',
    borderRadius: '6px',
    cursor: 'pointer',
    whiteSpace: 'nowrap',
  },
  pendingText: {
    margin: '8px 0 0',
    fontSize: '13px',
    color: '#6b7280',
    fontStyle: 'italic',
  },
  errorText: {
    margin: '8px 0 0',
    fontSize: '13px',
    color: '#d32f2f',
  },
  successText: {
    margin: '8px 0 0',
    fontSize: '13px',
    color: '#2e7d32',
  },
  listBox: {
    background: '#fff',
    border: '1px solid #e0e2e8',
    borderRadius: '8px',
    padding: '16px',
  },
  emptyText: {
    margin: 0,
    fontSize: '14px',
    color: '#6b7280',
  },
  table: {
    width: '100%',
    borderCollapse: 'collapse',
    fontSize: '13px',
  },
  th: {
    textAlign: 'left' as const,
    padding: '8px 12px',
    borderBottom: '1px solid #e0e2e8',
    fontWeight: 600,
    color: '#6b7280',
    fontSize: '12px',
    textTransform: 'uppercase' as const,
  },
  tr: {
    borderBottom: '1px solid #f0f1f4',
  },
  td: {
    padding: '10px 12px',
    verticalAlign: 'top' as const,
  },
  code: {
    fontFamily: 'monospace',
    fontSize: '12px',
    background: '#f0f1f4',
    padding: '2px 5px',
    borderRadius: '3px',
  },
  permList: {
    fontSize: '11px',
    color: '#6b7280',
  },
  actionButtons: {
    display: 'flex',
    gap: '6px',
  },
  openButton: {
    padding: '4px 10px',
    fontSize: '12px',
    background: '#17a2b8',
    color: '#fff',
    border: 'none',
    borderRadius: '4px',
    cursor: 'pointer',
  },
  uninstallButton: {
    padding: '4px 10px',
    fontSize: '12px',
    background: 'transparent',
    color: '#d32f2f',
    border: '1px solid #d32f2f',
    borderRadius: '4px',
    cursor: 'pointer',
  },
}

export default SnapsStore
