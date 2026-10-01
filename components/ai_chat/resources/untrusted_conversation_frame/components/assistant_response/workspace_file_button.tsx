// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import { isWorkspaceContent } from '../../../common/conversation_history_utils'
import { AttachmentItem } from '../../../page/components/attachment_item'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './workspace_file_button.module.scss'

// Renders the `::workspace[path/to/file]` markdown directive: a clickable
// attachment item that opens the file in a lightbox viewer.
export default function WorkspaceFileButton(props: { path: string }) {
  const context = useUntrustedConversationContext()

  // Check if a workspace is attached
  const hasWorkspace = React.useMemo(() => {
    return context.associatedContent?.some(isWorkspaceContent)
  }, [context.associatedContent])

  const handleClick = React.useCallback(() => {
    context.parentUiFrame?.showWorkspaceFileLightbox(props.path)
  }, [props.path, context.parentUiFrame])

  if (!hasWorkspace) {
    return null
  }

  return (
    <AttachmentItem
      icon={<Icon name='file' />}
      title={props.path}
      subtitle=''
      onClick={handleClick}
      className={styles.attachmentItem}
    />
  )
}
