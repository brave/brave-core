// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'

import { BraveWallet } from '../../../constants/types'
import {
  useNotifySnapInstallRequestProcessedMutation,
} from '../../../common/slices/api.slice'

interface Props {
  pending: BraveWallet.PendingSnapInstall
}

function formatBytes(bytes: bigint): string {
  const n = Number(bytes)
  if (n < 1024) return `${n} B`
  if (n < 1024 * 1024) return `${(n / 1024).toFixed(1)} KB`
  return `${(n / (1024 * 1024)).toFixed(1)} MB`
}

export const SnapInstallPanel = ({ pending }: Props) => {
  const [notifyProcessed] = useNotifySnapInstallRequestProcessedMutation()
  const [showManifest, setShowManifest] = React.useState(false)

  const manifest = pending.manifest ?? null
  const state = pending.state

  const handleApprove = () => {
    notifyProcessed({ approved: true })
  }

  const handleReject = () => {
    notifyProcessed({ approved: false })
  }

  // Installing / persisting
  if (
    state === BraveWallet.SnapInstallState.kInstalling
  ) {
    return (
      <div style={styles.centered}>
        <ProgressRing mode='indeterminate' />
        <p style={styles.statusText}>
          {manifest
            ? `Installing ${manifest.proposedName}…`
            : 'Downloading snap…'}
        </p>
      </div>
    )
  }

  // Success
  if (state === BraveWallet.SnapInstallState.kSuccess) {
    return (
      <div style={styles.centered}>
        <div style={styles.checkmark}>✓</div>
        <p style={styles.statusText}>
          {manifest ? `${manifest.proposedName} installed!` : 'Snap installed!'}
        </p>
      </div>
    )
  }

  // Failed
  if (state === BraveWallet.SnapInstallState.kFailed) {
    return (
      <div style={styles.page}>
        <h3 style={styles.errorTitle}>Installation failed</h3>
        <p style={styles.errorText}>{pending.error ?? 'Unknown error'}</p>
        <button
          style={styles.rejectBtn}
          onClick={handleReject}
        >
          Close
        </button>
      </div>
    )
  }

  // Pending approval
  if (!manifest) {
    return null
  }

  return (
    <div style={styles.page}>
      <div style={styles.header}>
        <h2 style={styles.snapName}>{manifest.proposedName}</h2>
        <span style={styles.snapId}>{manifest.snapId}</span>
        <span style={styles.version}>v{manifest.version}</span>
      </div>

      {manifest.description && (
        <p style={styles.description}>{manifest.description}</p>
      )}

      <div style={styles.section}>
        <span style={styles.label}>Size</span>
        <span style={styles.value}>
          {formatBytes(manifest.bundleSizeBytes)}
        </span>
      </div>

      {manifest.permissions.length > 0 && (
        <div style={styles.section}>
          <span style={styles.label}>Permissions</span>
          <div style={styles.chips}>
            {manifest.permissions.map((p) => (
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

      <div style={styles.section}>
        <button
          style={styles.toggleBtn}
          onClick={() => setShowManifest((v) => !v)}
        >
          {showManifest ? 'Hide' : 'Show'} manifest JSON
        </button>
        {showManifest && (
          <pre style={styles.manifest}>{manifest.manifestJson}</pre>
        )}
      </div>

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
          Approve
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
  centered: {
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    justifyContent: 'center',
    height: '100%',
    gap: '16px',
  },
  header: {
    display: 'flex',
    flexDirection: 'column',
    gap: '4px',
    borderBottom: '1px solid #e0e2e8',
    paddingBottom: '12px',
  },
  snapName: {
    margin: 0,
    fontSize: '18px',
    fontWeight: 600,
    color: '#1d1f25',
  },
  snapId: {
    fontSize: '11px',
    color: '#6b7280',
    fontFamily: 'monospace',
  },
  version: {
    fontSize: '12px',
    color: '#6b7280',
  },
  description: {
    margin: 0,
    fontSize: '13px',
    color: '#374151',
    lineHeight: 1.5,
  },
  section: {
    display: 'flex',
    flexDirection: 'column',
    gap: '6px',
  },
  label: {
    fontSize: '11px',
    fontWeight: 600,
    color: '#6b7280',
    textTransform: 'uppercase',
    letterSpacing: '0.05em',
  },
  value: {
    fontSize: '13px',
    color: '#1d1f25',
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
  manifest: {
    margin: 0,
    padding: '8px',
    fontSize: '11px',
    background: '#f8f9fa',
    border: '1px solid #e0e2e8',
    borderRadius: '4px',
    overflowX: 'auto',
    maxHeight: '160px',
    overflowY: 'auto',
    whiteSpace: 'pre-wrap',
    wordBreak: 'break-all',
  },
  toggleBtn: {
    alignSelf: 'flex-start',
    padding: '4px 10px',
    fontSize: '12px',
    background: 'transparent',
    color: '#6b7280',
    border: '1px solid #e0e2e8',
    borderRadius: '4px',
    cursor: 'pointer',
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
  statusText: {
    margin: 0,
    fontSize: '14px',
    color: '#374151',
  },
  checkmark: {
    fontSize: '48px',
    color: '#22c55e',
  },
  errorTitle: {
    margin: 0,
    fontSize: '16px',
    fontWeight: 600,
    color: '#d32f2f',
  },
  errorText: {
    margin: 0,
    fontSize: '13px',
    color: '#374151',
    wordBreak: 'break-word',
  },
}

export default SnapInstallPanel
