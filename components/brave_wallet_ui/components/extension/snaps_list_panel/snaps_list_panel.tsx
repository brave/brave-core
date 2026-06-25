// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { BraveWallet } from '../../../constants/types'
import { useGetInstalledSnapsQuery } from '../../../common/slices/api.slice'
import { useAppDispatch } from '../../../common/hooks/use-redux'
import { PanelActions } from '../../../panel/actions'

const hasPageHomePermission = (snap: BraveWallet.SnapInstallData) =>
  snap.manifest?.permissions?.includes('endowment:page-home') ?? false

export const SnapsListPanel = () => {
  const dispatch = useAppDispatch()
  const { data: snaps = [] as BraveWallet.SnapInstallData[], isLoading } =
    useGetInstalledSnapsQuery()

  const handleOpen = (snapId: string) => {
    dispatch(PanelActions.navigateToSnap(snapId))
  }

  const handleOpenDetails = (snapId: string) => {
    dispatch(PanelActions.navigateToSnapDetails(snapId))
  }

  const handleOpenInTab = (snapId: string) => {
    chrome.tabs.create({
      url: `brave://wallet/snap/${encodeURIComponent(snapId)}`,
    })
  }

  const handleOpenStore = () => {
    chrome.tabs.create({ url: 'brave://wallet/snaps-store' })
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
              <div style={styles.info}>
                <span style={styles.name}>
                  {snap.manifest?.proposedName || snap.snapId}
                </span>
                <span style={styles.id}>{snap.snapId}</span>
              </div>

              <div style={styles.actions}>
                {hasPageHomePermission(snap) && (
                  <button
                    style={styles.openBtn}
                    title='Open in panel'
                    onClick={() => handleOpen(snap.snapId)}
                  >
                    Open
                  </button>
                )}
                <button
                  style={styles.tabBtn}
                  title='Open in browser tab'
                  onClick={() => handleOpenInTab(snap.snapId)}
                >
                  ⎋ Tab
                </button>
                <button
                  style={styles.detailsBtn}
                  title='Show details'
                  onClick={() => handleOpenDetails(snap.snapId)}
                >
                  Details
                </button>
              </div>
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
  detailsBtn: {
    padding: '4px 8px',
    fontSize: '11px',
    background: 'transparent',
    color: '#374151',
    border: '1px solid #c0c4d0',
    borderRadius: '4px',
    cursor: 'pointer',
    whiteSpace: 'nowrap',
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
