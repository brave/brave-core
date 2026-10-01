// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import * as Mojom from '../../../common/mojom'
import { AttachmentItem } from '../../../page/components/attachment_item'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './workspace_file_button.module.scss'

// Extracts just the filename from a path (e.g., "src/foo/bar.ts" -> "bar.ts")
function getFilename(path: string): string {
  const parts = path.split('/')
  return parts[parts.length - 1] || path
}

// Renders the `::workspace[path/to/file]` markdown directive: a clickable
// attachment item that opens the file in a lightbox viewer.
export default function WorkspaceFileButton(props: { path: string }) {
  const context = useUntrustedConversationContext()
  const filename = getFilename(props.path)

  // Check if a workspace is attached
  const hasWorkspace = React.useMemo(() => {
    return context.associatedContent?.some(
      (c) => c.contentType === Mojom.ContentType.Workspace,
    )
  }, [context.associatedContent])

  const handleClick = React.useCallback(() => {
    context.parentUiFrame?.showWorkspaceFileLightbox(props.path)
  }, [props.path, context.parentUiFrame])

  // If no workspace is attached, render a non-clickable version
  if (!hasWorkspace) {
    return (
      <span
        className={styles.workspaceFileButton}
        title={props.path}
        data-testid='workspace-file-button'
      >
        <Icon name='file' />
        <span className={styles.filename}>{filename}</span>
      </span>
    )
  }

  return (
    <AttachmentItem
      icon={<Icon name='file' />}
      title={filename}
      subtitle={props.path}
      onClick={handleClick}
      className={styles.attachmentItem}
    />
  )
}
