// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Toggle from '@brave/leo/react/toggle'

import {
  useGetInstalledSnapsQuery,
  useUninstallSnapMutation,
  useGetSnapConnectedOriginsQuery,
  useDisconnectSnapOriginMutation,
  useSetSnapEnabledMutation,
} from '../../../common/slices/api.slice'
import { getSnapManifestForDisplay } from '../../../common/snap/snap_manifest_utils'
import { useAppDispatch } from '../../../common/hooks/use-redux'
import { PanelActions } from '../../../panel/actions'

interface Props {
  snapId: string
}

function formatInstallOrigin(installOrigin: string | undefined): string {
  return installOrigin || 'Brave Wallet'
}

const ConnectedOriginsList = ({ snapId }: { snapId: string }) => {
  const { data: origins = [], isLoading } = useGetSnapConnectedOriginsQuery({
    snapId,
  })
  const [disconnectSnapOrigin] = useDisconnectSnapOriginMutation()

  return (
    <div style={styles.connectionsSection}>
      <span style={styles.connectionsLabel}>Connected sites</span>
      {isLoading ? (
        <span style={styles.connectionsEmpty}>Loading…</span>
      ) : origins.length === 0 ? (
        <span style={styles.connectionsEmpty}>No connected sites.</span>
      ) : (
        <ul style={styles.originList}>
          {origins.map((origin) => (
            <li
              key={origin}
              style={styles.originItem}
            >
              <span style={styles.originName}>{origin}</span>
              <button
                style={styles.disconnectBtn}
                title='Disconnect'
                onClick={() => disconnectSnapOrigin({ origin, snapId })}
              >
                Disconnect
              </button>
            </li>
          ))}
        </ul>
      )}
    </div>
  )
}

export const SnapDetailsPanel = ({ snapId }: Props) => {
  const dispatch = useAppDispatch()
  const { data: snaps = [], isLoading } = useGetInstalledSnapsQuery()
  const [uninstallSnap] = useUninstallSnapMutation()
  const [setSnapEnabled] = useSetSnapEnabledMutation()
  const [confirmRemove, setConfirmRemove] = React.useState(false)

  const snap = snaps.find((entry) => entry.snapId === snapId)

  const handleBack = () => {
    dispatch(PanelActions.navigateTo('snaps'))
  }

  const handleConfirmRemove = async () => {
    await uninstallSnap({ snapId })
    dispatch(PanelActions.navigateTo('snaps'))
  }

  if (isLoading) {
    return (
      <div style={styles.page}>
        <div style={styles.header}>
          <button
            style={styles.backBtn}
            onClick={handleBack}
          >
            ← Snaps
          </button>
        </div>
        <div style={styles.empty}>Loading…</div>
      </div>
    )
  }

  if (!snap) {
    return (
      <div style={styles.page}>
        <div style={styles.header}>
          <button
            style={styles.backBtn}
            onClick={handleBack}
          >
            ← Snaps
          </button>
        </div>
        <div style={styles.empty}>Snap not found.</div>
      </div>
    )
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
      </div>

      <div style={styles.content}>
        <h3 style={styles.title}>
          {snap.manifest?.proposedName || snap.snapId}
        </h3>
        <span style={styles.snapId}>{snap.snapId}</span>

        <div style={styles.enabledRow}>
          <span style={styles.enabledLabel}>Enabled</span>
          <Toggle
            checked={snap.enabled}
            onChange={({ checked }) =>
              setSnapEnabled({ snapId: snap.snapId, enabled: checked })
            }
            size='small'
          />
        </div>

        <div style={styles.infoSection}>
          <span style={styles.infoLabel}>Install origin</span>
          <span style={styles.infoValue}>
            {formatInstallOrigin(snap.installOrigin)}
          </span>
        </div>

        <pre style={styles.manifest}>
          {JSON.stringify(
            {
              snapId: snap.snapId,
              version: snap.version,
              installOrigin: formatInstallOrigin(snap.installOrigin),
              enabled: snap.enabled,
              manifest: getSnapManifestForDisplay(snap.manifest),
            },
            null,
            2,
          )}
        </pre>

        <ConnectedOriginsList snapId={snap.snapId} />

        <div style={styles.detailsActions}>
          {confirmRemove ? (
            <div style={styles.confirmRemove}>
              <span style={styles.confirmRemoveText}>
                Remove {snap.manifest?.proposedName || snap.snapId}? This cannot
                be undone.
              </span>
              <div style={styles.confirmRemoveBtns}>
                <button
                  style={styles.cancelBtn}
                  onClick={() => setConfirmRemove(false)}
                >
                  Cancel
                </button>
                <button
                  style={styles.removeBtn}
                  onClick={handleConfirmRemove}
                >
                  Remove
                </button>
              </div>
            </div>
          ) : (
            <button
              style={styles.removeBtn}
              onClick={() => setConfirmRemove(true)}
            >
              Remove
            </button>
          )}
        </div>
      </div>
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
  content: {
    flex: 1,
    display: 'flex',
    flexDirection: 'column',
    gap: '8px',
    padding: '16px',
    overflowY: 'auto',
  },
  title: {
    margin: 0,
    fontSize: '15px',
    fontWeight: 600,
    color: '#1d1f25',
  },
  snapId: {
    fontSize: '10px',
    color: '#6b7280',
    fontFamily: 'monospace',
    overflow: 'hidden',
    textOverflow: 'ellipsis',
    whiteSpace: 'nowrap',
  },
  enabledRow: {
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'space-between',
    gap: '8px',
    padding: '8px 10px',
    background: '#f8f9fa',
    border: '1px solid #e0e2e8',
    borderRadius: '4px',
  },
  enabledLabel: {
    fontSize: '12px',
    fontWeight: 500,
    color: '#374151',
  },
  infoSection: {
    display: 'flex',
    flexDirection: 'column',
    gap: '4px',
    padding: '8px 10px',
    background: '#f8f9fa',
    border: '1px solid #e0e2e8',
    borderRadius: '4px',
  },
  infoLabel: {
    fontSize: '10px',
    fontWeight: 600,
    color: '#6b7280',
    textTransform: 'uppercase',
    letterSpacing: '0.05em',
  },
  infoValue: {
    fontSize: '11px',
    color: '#374151',
    fontFamily: 'monospace',
    overflow: 'hidden',
    textOverflow: 'ellipsis',
    whiteSpace: 'nowrap',
  },
  manifest: {
    margin: 0,
    padding: '8px',
    fontSize: '10px',
    fontFamily: 'monospace',
    background: '#f8f9fa',
    border: '1px solid #e0e2e8',
    borderRadius: '4px',
    overflowX: 'auto',
    whiteSpace: 'pre-wrap' as const,
    wordBreak: 'break-all' as const,
    color: '#374151',
    lineHeight: 1.5,
  },
  connectionsSection: {
    display: 'flex',
    flexDirection: 'column',
    gap: '6px',
  },
  connectionsLabel: {
    fontSize: '10px',
    fontWeight: 600,
    color: '#6b7280',
    textTransform: 'uppercase',
    letterSpacing: '0.05em',
  },
  connectionsEmpty: {
    fontSize: '11px',
    color: '#6b7280',
  },
  originList: {
    listStyle: 'none',
    margin: 0,
    padding: 0,
    display: 'flex',
    flexDirection: 'column',
    gap: '4px',
  },
  originItem: {
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'space-between',
    gap: '6px',
    padding: '6px 8px',
    background: '#fff',
    border: '1px solid #e0e2e8',
    borderRadius: '4px',
  },
  originName: {
    fontSize: '11px',
    color: '#374151',
    fontFamily: 'monospace',
    overflow: 'hidden',
    textOverflow: 'ellipsis',
    whiteSpace: 'nowrap',
    minWidth: 0,
    flex: 1,
  },
  disconnectBtn: {
    padding: '4px 8px',
    fontSize: '11px',
    background: 'transparent',
    color: '#d32f2f',
    border: '1px solid #d32f2f',
    borderRadius: '4px',
    cursor: 'pointer',
    whiteSpace: 'nowrap',
    flexShrink: 0,
  },
  detailsActions: {
    display: 'flex',
    justifyContent: 'flex-end',
    marginTop: '4px',
  },
  confirmRemove: {
    width: '100%',
    display: 'flex',
    flexDirection: 'column',
    gap: '8px',
    padding: '8px 10px',
    background: '#fff',
    border: '1px solid #e0e2e8',
    borderRadius: '4px',
  },
  confirmRemoveText: {
    fontSize: '11px',
    color: '#374151',
    lineHeight: 1.4,
  },
  confirmRemoveBtns: {
    display: 'flex',
    justifyContent: 'flex-end',
    gap: '6px',
  },
  cancelBtn: {
    padding: '4px 10px',
    fontSize: '11px',
    background: 'transparent',
    color: '#374151',
    border: '1px solid #c0c4d0',
    borderRadius: '4px',
    cursor: 'pointer',
    whiteSpace: 'nowrap',
  },
  removeBtn: {
    padding: '4px 10px',
    fontSize: '11px',
    background: 'transparent',
    color: '#d32f2f',
    border: '1px solid #d32f2f',
    borderRadius: '4px',
    cursor: 'pointer',
    whiteSpace: 'nowrap',
  },
  empty: {
    padding: '16px',
    fontSize: '13px',
    color: '#6b7280',
  },
}

export default SnapDetailsPanel
