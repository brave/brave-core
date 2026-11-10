/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import { requestWorkspaceZip } from '../../../leo_workspace/download'
import styles from './style.module.scss'

export interface WorkspaceFileInfo {
  workspaceUrl: string
  filePath: string
}

interface Props {
  file: WorkspaceFileInfo | null
  onClose: () => void
  // Opens the file in the current tab.
  onOpen: (file: WorkspaceFileInfo) => void
  // Replaces the workspace viewer iframe, e.g. so storybook can render
  // placeholder content instead of loading a chrome-untrusted:// page.
  renderViewer?: (file: WorkspaceFileInfo) => React.ReactNode
  // Title of the conversation the workspace belongs to. The downloaded zip is
  // named after it, falling back to the workspace folder's name.
  conversationTitle?: string
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
 * Maps the workspace URL to the origin of the workspace page.
 * Workspace URL: workspace://<uuid>
 * Origin: chrome-untrusted://<uuid>.leo-workspace
 */
function getWorkspaceOrigin(workspaceUrl: string): string {
  try {
    const url = new URL(workspaceUrl)
    if (url.protocol !== 'workspace:' || !url.hostname) {
      return ''
    }
    return `chrome-untrusted://${url.hostname}.leo-workspace`
  } catch {
    return ''
  }
}

/**
 * Builds the iframe URL from the workspace origin and file path.
 * The workspace page handles the #file= fragment to display the file.
 * Iframe URL: chrome-untrusted://<uuid>.leo-workspace/#file=<path>
 */
function buildViewerUrl(workspaceOrigin: string, filePath: string): string {
  return `${workspaceOrigin}/#file=${encodeURIComponent(filePath)}`
}

const MAX_DOWNLOAD_NAME_LENGTH = 100

/**
 * Turns a conversation title into a hyphenated file name stem, e.g.
 * "My Cool Site!" -> "my-cool-site". Letters and digits in any script are
 * kept; everything else collapses to a single hyphen. Returns '' if nothing
 * usable is left.
 */
export function hyphenateTitle(title: string): string {
  const hyphenated = title
    .normalize('NFKC')
    .toLocaleLowerCase()
    .replace(/[^\p{L}\p{N}]+/gu, '-')
  // Truncate by code point so a surrogate pair is never split.
  return Array.from(hyphenated)
    .slice(0, MAX_DOWNLOAD_NAME_LENGTH)
    .join('')
    .replace(/^-+|-+$/g, '')
}

function downloadFile(file: File, name: string) {
  const url = URL.createObjectURL(file)
  const link = document.createElement('a')
  link.href = url
  link.download = name
  document.body.appendChild(link)
  link.click()
  document.body.removeChild(link)
  // Revoke on a later task so the download has picked up the blob.
  setTimeout(() => URL.revokeObjectURL(url), 0)
}

export default function WorkspaceFileLightbox(props: Props) {
  const { file, onClose } = props

  const workspaceOrigin = React.useMemo(() => {
    if (!file) {
      return ''
    }
    return getWorkspaceOrigin(file.workspaceUrl)
  }, [file])

  const viewerUrl = React.useMemo(() => {
    if (!file || !workspaceOrigin) {
      return ''
    }
    return buildViewerUrl(workspaceOrigin, file.filePath)
  }, [file, workspaceOrigin])

  const iframeRef = React.useRef<HTMLIFrameElement>(null)
  // The workspace page installs its message handler before its load event, so
  // a download can only be requested once the iframe has loaded.
  const [isViewerLoaded, setIsViewerLoaded] = React.useState(false)
  const [isDownloading, setIsDownloading] = React.useState(false)
  const downloadAbortRef = React.useRef<AbortController | null>(null)

  // Reset when the lightbox closes or switches workspace, abandoning any
  // download in flight. Switching file within the same workspace only changes
  // the iframe's fragment, which doesn't reload it (or fire onLoad again).
  React.useEffect(() => {
    setIsViewerLoaded(false)
    setIsDownloading(false)
    return () => {
      downloadAbortRef.current?.abort()
      downloadAbortRef.current = null
    }
  }, [workspaceOrigin])

  const handleDownload = React.useCallback(async () => {
    const target = iframeRef.current?.contentWindow
    if (!target || !workspaceOrigin || downloadAbortRef.current) {
      return
    }
    const controller = new AbortController()
    downloadAbortRef.current = controller
    setIsDownloading(true)
    try {
      const zip = await requestWorkspaceZip(
        target,
        workspaceOrigin,
        controller.signal,
      )
      const stem = hyphenateTitle(props.conversationTitle ?? '')
      downloadFile(zip, stem ? `${stem}.zip` : zip.name)
    } catch (e) {
      if (!controller.signal.aborted) {
        console.error('Failed to download workspace:', e)
      }
    } finally {
      if (downloadAbortRef.current === controller) {
        downloadAbortRef.current = null
        setIsDownloading(false)
      }
    }
  }, [workspaceOrigin, props.conversationTitle])

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
            <div className={styles.titleRow}>
              <span
                className={styles.title}
                title={file.filePath}
              >
                {filename}
              </span>
              <Button
                fab
                kind='plain-faint'
                size='small'
                className={styles.actionButton}
                title={getLocale(
                  S.CHAT_UI_WORKSPACE_FILE_LIGHTBOX_OPEN_BUTTON_LABEL,
                )}
                aria-label={getLocale(
                  S.CHAT_UI_WORKSPACE_FILE_LIGHTBOX_OPEN_BUTTON_LABEL,
                )}
                onClick={() => props.onOpen(file)}
              >
                <Icon name='launch' />
              </Button>
              {!props.renderViewer && (
                <Button
                  fab
                  kind='plain-faint'
                  size='small'
                  className={styles.actionButton}
                  title={getLocale(
                    S.CHAT_UI_WORKSPACE_FILE_LIGHTBOX_DOWNLOAD_BUTTON_LABEL,
                  )}
                  aria-label={getLocale(
                    S.CHAT_UI_WORKSPACE_FILE_LIGHTBOX_DOWNLOAD_BUTTON_LABEL,
                  )}
                  isDisabled={!isViewerLoaded}
                  isLoading={isDownloading}
                  onClick={handleDownload}
                >
                  <Icon name='download' />
                </Button>
              )}
            </div>
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
                ref={iframeRef}
                src={viewerUrl}
                className={styles.iframe}
                title={filename}
                onLoad={() => setIsViewerLoaded(true)}
              />
            )}
          </div>
        </div>
      )}
    </Dialog>
  )
}
