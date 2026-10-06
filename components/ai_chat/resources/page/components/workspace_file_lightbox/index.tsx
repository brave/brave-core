/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Dialog from '@brave/leo/react/dialog'
import styles from './style.module.scss'

export interface WorkspaceFileInfo {
  workspaceUrl: string
  filePath: string
}

interface Props {
  file: WorkspaceFileInfo | null
  onClose: () => void
  // Replaces the workspace viewer iframe, e.g. so storybook can render
  // placeholder content instead of loading a chrome-untrusted:// page.
  renderViewer?: (file: WorkspaceFileInfo) => React.ReactNode
}

const DIALOG_WIDTH = 800
const DIALOG_HEIGHT = 600

/**
 * Extracts just the filename from a path (e.g., "src/foo/bar.ts" -> "bar.ts")
 */
function getFilename(path: string): string {
  const parts = path.split('/')
  return parts[parts.length - 1] || path
}

/**
 * Builds the iframe URL from the workspace URL and file path.
 * The workspace page handles the #file= fragment to display the file.
 * Workspace URL: workspace://<uuid>
 * Iframe URL: chrome-untrusted://<uuid>.leo-workspace/#file=<path>
 */
function buildViewerUrl(workspaceUrl: string, filePath: string): string {
  try {
    const url = new URL(workspaceUrl)
    if (url.protocol !== 'workspace:' || !url.hostname) {
      return ''
    }
    return `chrome-untrusted://${url.hostname}.leo-workspace/#file=${encodeURIComponent(filePath)}`
  } catch {
    return ''
  }
}

export default function WorkspaceFileLightbox(props: Props) {
  const { file, onClose } = props

  const viewerUrl = React.useMemo(() => {
    if (!file) {
      return ''
    }
    return buildViewerUrl(file.workspaceUrl, file.filePath)
  }, [file])

  const filename = React.useMemo(() => {
    if (!file) {
      return ''
    }
    return getFilename(file.filePath)
  }, [file])

  const dialogStyle = `--leo-dialog-width: ${DIALOG_WIDTH}px`

  return (
    <Dialog
      isOpen={!!file}
      showClose
      escapeCloses
      backdropClickCloses
      onClose={onClose}
      className={styles.dialog}
      style={dialogStyle}
    >
      {file && viewerUrl && (
        <div className={styles.card}>
          <div className={styles.header}>
            <span
              className={styles.title}
              title={file.filePath}
            >
              {filename}
            </span>
            <span className={styles.subtitle}>{file.filePath}</span>
          </div>
          <div
            className={styles.iframeContainer}
            style={{ height: `${DIALOG_HEIGHT}px` }}
          >
            {props.renderViewer ? (
              props.renderViewer(file)
            ) : (
              <iframe
                src={viewerUrl}
                className={styles.iframe}
                title={filename}
              />
            )}
          </div>
        </div>
      )}
    </Dialog>
  )
}
