// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/nala/react/button'
import Icon from '@brave/nala/react/icon'
import { getLocale } from '$web-common/locale'
import { isWorkspaceContent } from '../../../common/conversation_history_utils'
import {
  buildViewerUrl,
  getFilename,
  getWorkspaceOrigin,
} from '../../../common/workspace_url'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './workspace_file_viewer.module.scss'

interface Props {
  path: string
  // Replaces the workspace viewer iframe, e.g. so storybook can render
  // placeholder content instead of loading a chrome-untrusted:// page.
  renderViewer?: (viewerUrl: string) => React.ReactNode
}

// Renders the `::workspace[path/to/file]{inline}` markdown directive: the
// file's contents embedded in the conversation, with a button to open it in
// the lightbox viewer.
export default function WorkspaceFileViewer(props: Props) {
  const context = useUntrustedConversationContext()

  const viewerUrl = React.useMemo(() => {
    const workspaceUrl =
      context.associatedContent?.find(isWorkspaceContent)?.url?.url
    const origin = workspaceUrl ? getWorkspaceOrigin(workspaceUrl) : ''
    return origin ? buildViewerUrl(origin, props.path) : ''
  }, [context.associatedContent, props.path])

  const handleExpand = React.useCallback(() => {
    context.parentUiFrame?.showWorkspaceFileLightbox(props.path)
  }, [props.path, context.parentUiFrame])

  if (!viewerUrl) {
    return null
  }

  const filename = getFilename(props.path)
  const expandLabel = getLocale(S.CHAT_UI_WORKSPACE_FILE_EXPAND_BUTTON_LABEL)

  return (
    <div
      className={styles.card}
      data-testid='workspace-file-viewer'
    >
      <div className={styles.header}>
        <div className={styles.titles}>
          <span
            className={styles.title}
            title={props.path}
          >
            {filename}
          </span>
          <span className={styles.subtitle}>{props.path}</span>
        </div>
        <Button
          fab
          kind='plain-faint'
          size='small'
          className={styles.actionButton}
          title={expandLabel}
          aria-label={expandLabel}
          onClick={handleExpand}
        >
          <Icon name='expand' />
        </Button>
      </div>
      <div className={styles.viewerContainer}>
        {props.renderViewer ? (
          props.renderViewer(viewerUrl)
        ) : (
          <iframe
            src={viewerUrl}
            className={styles.iframe}
            title={filename}
            // A conversation can show many files, each loading its own copy of
            // the workspace page, so only load the ones scrolled into view.
            loading='lazy'
          />
        )}
      </div>
    </div>
  )
}
