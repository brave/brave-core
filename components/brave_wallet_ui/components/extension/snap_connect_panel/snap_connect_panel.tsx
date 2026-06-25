// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { BraveWallet } from '../../../constants/types'
import {
  useNotifySnapConnectionRequestProcessedMutation, //
} from '../../../common/slices/api.slice'

interface Props {
  pending: BraveWallet.PendingSnapConnection
}

export const SnapConnectPanel = ({ pending }: Props) => {
  const [notifyProcessed] = useNotifySnapConnectionRequestProcessedMutation()

  const snapInfo = pending.snapInfo ?? null
  const snapName =
    snapInfo?.manifest?.proposedName || snapInfo?.snapId || pending.snapId

  const handleApprove = () => {
    notifyProcessed({ approved: true })
  }

  const handleReject = () => {
    notifyProcessed({ approved: false })
  }

  return (
    <div style={styles.page}>
      <div style={styles.header}>
        <h2 style={styles.title}>Connection request</h2>
        <p style={styles.subtitle}>
          A site wants to connect to a snap in your wallet.
        </p>
      </div>

      <div style={styles.section}>
        <span style={styles.label}>Website</span>
        <span style={styles.origin}>{pending.origin}</span>
      </div>

      <div style={styles.section}>
        <span style={styles.label}>Snap</span>
        <span style={styles.snapName}>{snapName}</span>
        <span style={styles.snapId}>{pending.snapId}</span>
      </div>

      {snapInfo && snapInfo.manifest.permissions.length > 0 && (
        <div style={styles.section}>
          <span style={styles.label}>This snap can</span>
          <div style={styles.chips}>
            {snapInfo.manifest.permissions.map((p: string) => (
              <span
                key={p}
                style={styles.chip}
              >
                {p}
              </span>
            ))}
          </div>
        </div>
      )}

      <div style={styles.actions}>
        <button
          style={styles.rejectBtn}
          onClick={handleReject}
        >
          Reject
        </button>
        <button
          style={styles.approveBtn}
          onClick={handleApprove}
        >
          Connect
        </button>
      </div>
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
    gap: '12px',
  },
  header: {
    display: 'flex',
    flexDirection: 'column',
    gap: '4px',
    borderBottom: '1px solid #e0e2e8',
    paddingBottom: '12px',
  },
  title: {
    margin: 0,
    fontSize: '18px',
    fontWeight: 600,
    color: '#1d1f25',
  },
  subtitle: {
    margin: 0,
    fontSize: '13px',
    color: '#6b7280',
  },
  section: {
    display: 'flex',
    flexDirection: 'column',
    gap: '4px',
  },
  label: {
    fontSize: '11px',
    fontWeight: 600,
    color: '#6b7280',
    textTransform: 'uppercase',
    letterSpacing: '0.05em',
  },
  origin: {
    fontSize: '14px',
    fontWeight: 500,
    color: '#1d1f25',
    wordBreak: 'break-all',
  },
  snapName: {
    fontSize: '14px',
    fontWeight: 500,
    color: '#1d1f25',
  },
  snapId: {
    fontSize: '11px',
    color: '#6b7280',
    fontFamily: 'monospace',
    wordBreak: 'break-all',
  },
  chips: {
    display: 'flex',
    flexWrap: 'wrap',
    gap: '4px',
  },
  chip: {
    padding: '2px 8px',
    fontSize: '11px',
    background: '#f3f4f6',
    border: '1px solid #e0e2e8',
    borderRadius: '12px',
    color: '#374151',
    fontFamily: 'monospace',
  },
  actions: {
    display: 'flex',
    gap: '8px',
    marginTop: 'auto',
    paddingTop: '8px',
  },
  rejectBtn: {
    flex: 1,
    padding: '10px',
    fontSize: '14px',
    fontWeight: 500,
    background: 'transparent',
    color: '#374151',
    border: '1px solid #e0e2e8',
    borderRadius: '8px',
    cursor: 'pointer',
  },
  approveBtn: {
    flex: 1,
    padding: '10px',
    fontSize: '14px',
    fontWeight: 500,
    background: '#fb542b',
    color: '#fff',
    border: 'none',
    borderRadius: '8px',
    cursor: 'pointer',
  },
}

export default SnapConnectPanel
