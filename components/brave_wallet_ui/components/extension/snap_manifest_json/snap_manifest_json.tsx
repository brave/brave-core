// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { BraveWallet } from '../../../constants/types'
import { getSnapManifestForDisplay } from '../../../common/snap/snap_manifest_utils'

interface Props {
  manifest: BraveWallet.SnapManifest | null | undefined
}

export const SnapManifestJson = ({ manifest }: Props) => {
  const display = getSnapManifestForDisplay(manifest)
  if (!display) {
    return null
  }

  return (
    <pre style={styles.manifest}>
      {JSON.stringify(display, null, 2)}
    </pre>
  )
}

const styles: Record<string, React.CSSProperties> = {
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
}

export default SnapManifestJson
