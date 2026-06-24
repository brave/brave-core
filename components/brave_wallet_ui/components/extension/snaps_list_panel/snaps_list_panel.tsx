// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { BraveWallet } from '../../../constants/types'
import {
  useGetInstalledSnapsQuery,
  useUninstallSnapMutation,
  useGetSnapConnectedOriginsQuery,
  useDisconnectSnapOriginMutation,
} from '../../../common/slices/api.slice'
import { useAppDispatch } from '../../../common/hooks/use-redux'
import { PanelActions } from '../../../panel/actions'

// Lists the origins currently connected to a snap, each with a disconnect
// button. Only mounted while the snap's manifest drawer is expanded, so the
// query runs lazily.
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

export const SnapsListPanel = () => {
  const dispatch = useAppDispatch()
  const { data: snaps = [] as BraveWallet.SnapInstallData[], isLoading } =
    useGetInstalledSnapsQuery()
  const [uninstallSnap] = useUninstallSnapMutation()
  const [expandedManifestId, setExpandedManifestId] = React.useState<
    string | null
  >(null)

  const handleOpen = (snapId: string) => {
    dispatch(PanelActions.navigateToSnap(snapId))
  }

  const handleOpenInTab = (snapId: string) => {
    chrome.tabs.create({
      url: `brave://wallet/snap/${encodeURIComponent(snapId)}`,
    })
  }

  const handleOpenStore = () => {
    chrome.tabs.create({ url: 'brave://wallet/snaps-store' })
  }

  const toggleManifest = (snapId: string) => {
    setExpandedManifestId((prev) => (prev === snapId ? null : snapId))
  }

  if (isLoading) {
    return <div style={styles.empty}>Loading…</div>
  }

  return (
    <div style={styles.page}>
      <h3 style={styles.heading}>Installed Snaps</h3>

      {snaps.length === 0 ? (
        <div style={styles.emptyState}>
          <p style={styles.emptyText}>No snaps installed.</p>
          <button
            style={styles.storeLink}
            onClick={handleOpenStore}
          >
            Browse the Snaps Store
          </button>
        </div>
      ) : (
        <ul style={styles.list}>
          {snaps.map((snap) => (
            <li
              key={snap.snapId}
              style={styles.item}
            >
              {/* Name + ID */}
              <div style={styles.info}>
                <span style={styles.name}>
                  {snap.manifest?.proposedName || snap.snapId}
                </span>
                <span style={styles.id}>{snap.snapId}</span>
              </div>

              {/* Action buttons */}
              <div style={styles.actions}>
                <button
                  style={styles.openBtn}
                  title='Open in panel'
                  onClick={() => handleOpen(snap.snapId)}
                >
                  Open
                </button>
                <button
                  style={styles.tabBtn}
                  title='Open in browser tab'
                  onClick={() => handleOpenInTab(snap.snapId)}
                >
                  ⎋ Tab
                </button>
                <button
                  style={
                    expandedManifestId === snap.snapId
                      ? styles.manifestBtnActive
                      : styles.manifestBtn
                  }
                  title='Show manifest'
                  onClick={() => toggleManifest(snap.snapId)}
                >
                  { }
                </button>
                <button
                  style={styles.uninstallBtn}
                  title='Uninstall'
                  onClick={() => uninstallSnap({ snapId: snap.snapId })}
                >
                  ✕
                </button>
              </div>

              {/* Manifest drawer */}
              {expandedManifestId === snap.snapId && (
                <>
                  <pre style={styles.manifest}>
                    {JSON.stringify(
                      {
                        snap_id: snap.snapId,
                        version: snap.version,
                        proposed_name: snap.manifest?.proposedName,
                        permissions: snap.manifest?.permissions,
                        enabled: snap.enabled,
                      },
                      null,
                      2,
                    )}
                  </pre>
                  <ConnectedOriginsList snapId={snap.snapId} />
                </>
              )}
            </li>
          ))}
        </ul>
      )}
    </div>
  )
}

const styles: Record<string, React.CSSProperties> = {
  page: {
    display: 'flex',
    flexDirection: 'column',
    height: '100%',
    padding: '16px',
    fontFamily: 'system-ui, sans-serif',
    overflowY: 'auto',
  },
  heading: {
    margin: '0 0 12px',
    fontSize: '15px',
    fontWeight: 600,
    color: '#1d1f25',
  },
  list: {
    listStyle: 'none',
    margin: 0,
    padding: 0,
    display: 'flex',
    flexDirection: 'column',
    gap: '8px',
  },
  item: {
    display: 'flex',
    flexWrap: 'wrap' as const,
    alignItems: 'center',
    justifyContent: 'space-between',
    padding: '10px 12px',
    background: '#f8f9fa',
    border: '1px solid #e0e2e8',
    borderRadius: '8px',
    gap: '6px',
  },
  info: {
    display: 'flex',
    flexDirection: 'column',
    gap: '2px',
    minWidth: 0,
    flex: 1,
  },
  name: {
    fontSize: '13px',
    fontWeight: 500,
    color: '#1d1f25',
    overflow: 'hidden',
    textOverflow: 'ellipsis',
    whiteSpace: 'nowrap',
  },
  id: {
    fontSize: '10px',
    color: '#6b7280',
    fontFamily: 'monospace',
    overflow: 'hidden',
    textOverflow: 'ellipsis',
    whiteSpace: 'nowrap',
  },
  actions: {
    display: 'flex',
    gap: '4px',
    flexShrink: 0,
  },
  openBtn: {
    padding: '4px 10px',
    fontSize: '11px',
    fontWeight: 500,
    background: '#17a2b8',
    color: '#fff',
    border: 'none',
    borderRadius: '4px',
    cursor: 'pointer',
    whiteSpace: 'nowrap',
  },
  tabBtn: {
    padding: '4px 8px',
    fontSize: '11px',
    background: 'transparent',
    color: '#374151',
    border: '1px solid #c0c4d0',
    borderRadius: '4px',
    cursor: 'pointer',
    whiteSpace: 'nowrap',
  },
  manifestBtn: {
    padding: '4px 8px',
    fontSize: '11px',
    background: 'transparent',
    color: '#374151',
    border: '1px solid #c0c4d0',
    borderRadius: '4px',
    cursor: 'pointer',
  },
  manifestBtnActive: {
    padding: '4px 8px',
    fontSize: '11px',
    background: '#e0e7ff',
    color: '#3730a3',
    border: '1px solid #818cf8',
    borderRadius: '4px',
    cursor: 'pointer',
  },
  uninstallBtn: {
    padding: '4px 8px',
    fontSize: '11px',
    background: 'transparent',
    color: '#d32f2f',
    border: '1px solid #d32f2f',
    borderRadius: '4px',
    cursor: 'pointer',
  },
  manifest: {
    width: '100%',
    margin: '4px 0 0',
    padding: '8px',
    fontSize: '10px',
    fontFamily: 'monospace',
    background: '#fff',
    border: '1px solid #e0e2e8',
    borderRadius: '4px',
    overflowX: 'auto',
    whiteSpace: 'pre-wrap' as const,
    wordBreak: 'break-all' as const,
    color: '#374151',
    lineHeight: 1.5,
  },
  connectionsSection: {
    width: '100%',
    margin: '8px 0 0',
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
  empty: {
    padding: '16px',
    fontSize: '13px',
    color: '#6b7280',
  },
  emptyState: {
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    gap: '12px',
    paddingTop: '24px',
  },
  emptyText: {
    margin: 0,
    fontSize: '14px',
    color: '#6b7280',
  },
  storeLink: {
    padding: '8px 16px',
    fontSize: '13px',
    fontWeight: 500,
    background: '#fb542b',
    color: '#fff',
    border: 'none',
    borderRadius: '6px',
    cursor: 'pointer',
  },
}

export default SnapsListPanel
