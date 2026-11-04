// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/nala/react/button'
import Dialog from '@brave/nala/react/dialog'
import { getLocale, formatLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useAIChat } from '../../state/ai_chat_context'
import ConnectorEditor from './connector_editor'
import ConnectorList from './connector_list'
import { getConnectorErrorMessage } from './connector_utils'
import styles from './style.module.scss'

/**
 * Manages the user's connectors: remote MCP servers whose tools Leo can use.
 * Shows what `AIChatContext.connectorsDialog` asks for.
 */
export default function ConnectorsModal() {
  const aiChatContext = useAIChat()
  const { connectorsDialog: dialog, setConnectorsDialog } = aiChatContext

  // An editor opened from the list goes back to it, rather than closing.
  const [opensList] = React.useState(dialog?.view === 'list')
  const [removingServerId, setRemovingServerId] = React.useState<string>()
  const [listError, setListError] = React.useState<string>()
  const [isEditorSaving, setIsEditorSaving] = React.useState(false)

  const { getServersData, isPlaceholderData, isError, error } =
    aiChatContext.api.useGetServers()
  // Placeholder data is only given while loading, not after a failure.
  const servers = getServersData ?? []
  const isLoading = isPlaceholderData && !isError
  const removeServer = aiChatContext.api.useRemoveServer()

  const close = () => setConnectorsDialog(null)
  const showList = () => {
    setRemovingServerId(undefined)
    if (opensList) {
      setConnectorsDialog({ view: 'list' })
    } else {
      close()
    }
  }

  const editingServerId =
    dialog?.view === 'editor' ? dialog.serverId : undefined
  const editingServer = servers.find((s) => s.id === editingServerId)
  const removingServer = servers.find((s) => s.id === removingServerId)

  // The connector may be removed elsewhere while it's open here.
  const isOpenServerGone =
    !isLoading
    && ((!!editingServerId && !editingServer)
      || (!!removingServerId && !removingServer))
  React.useEffect(() => {
    if (isOpenServerGone) {
      showList()
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [isOpenServerGone])

  if (!dialog) {
    return null
  }

  const confirmRemove = (serverId: string) => {
    setListError(undefined)
    const onDone = (error: unknown) => {
      if (error !== null) {
        setListError(getConnectorErrorMessage(error))
      }
      setRemovingServerId(undefined)
    }
    removeServer.mutate([serverId], { onSuccess: onDone, onError: onDone })
  }

  let title: string
  let content: React.ReactNode
  if (removingServer) {
    title = getLocale(S.CHAT_UI_CONNECTOR_REMOVE_TITLE)
    content = (
      <ConnectorRemoveConfirm
        server={removingServer}
        isRemoving={removeServer.isPending}
        onCancel={() => setRemovingServerId(undefined)}
        onRemove={() => confirmRemove(removingServer.id)}
      />
    )
  } else if (dialog.view === 'editor') {
    title = getLocale(
      editingServerId
        ? S.CHAT_UI_CONNECTOR_EDIT_TITLE
        : S.CHAT_UI_CONNECTOR_ADD_TITLE,
    )
    content =
      editingServerId && !editingServer ? null : (
        <ConnectorEditor
          key={editingServerId}
          server={editingServer}
          onCancel={showList}
          onSaved={showList}
          onSavingChange={setIsEditorSaving}
        />
      )
  } else {
    title = getLocale(S.CHAT_UI_CONNECTORS_TITLE)
    content = (
      <ConnectorList
        // The list is unknown if it couldn't be fetched.
        servers={getServersData}
        isLoading={isLoading}
        error={isError ? getConnectorErrorMessage(error) : listError}
        onError={setListError}
        onAdd={() => {
          setListError(undefined)
          setConnectorsDialog({ view: 'editor' })
        }}
        onEdit={(serverId) => {
          setListError(undefined)
          setConnectorsDialog({ view: 'editor', serverId })
        }}
        onRemove={(serverId) => {
          setListError(undefined)
          setRemovingServerId(serverId)
        }}
      />
    )
  }

  const isList = dialog.view === 'list' && !removingServer
  // A save's later steps only run while the editor is open.
  const canLeave = !isEditorSaving

  return (
    <Dialog
      isOpen
      showClose={canLeave}
      showBack={!isList && opensList && canLeave}
      escapeCloses={canLeave}
      onBack={showList}
      onClose={close}
      // Unsaved changes in the editor shouldn't be lost to a stray click.
      backdropClickCloses={isList}
      className={styles.dialog}
    >
      <div
        slot='title'
        className={styles.title}
      >
        {title}
      </div>
      {content}
    </Dialog>
  )
}

function ConnectorRemoveConfirm(props: {
  server: Mojom.MCPServer
  isRemoving: boolean
  onCancel: () => void
  onRemove: () => void
}) {
  const cancelRef = React.useRef<HTMLElement>(null)

  // Focus starts on the choice that keeps the connector.
  React.useEffect(() => {
    const button = cancelRef.current
    if (!button) {
      return
    }
    setTimeout(() => button.focus())
  }, [])

  return (
    <>
      <div className={styles.body}>
        <div className={styles.description}>
          {formatLocale(S.CHAT_UI_CONNECTOR_REMOVE_DESCRIPTION, {
            $1: props.server.name,
          })}
        </div>
      </div>
      <div slot='actions'>
        <Button
          ref={cancelRef}
          kind='plain-faint'
          onClick={props.onCancel}
        >
          {getLocale(S.CHAT_UI_CANCEL_BUTTON_LABEL)}
        </Button>
        <Button
          kind='filled'
          className={styles.removeButton}
          isLoading={props.isRemoving}
          onClick={props.onRemove}
        >
          {getLocale(S.CHAT_UI_CONNECTOR_REMOVE_BUTTON_LABEL)}
        </Button>
      </div>
    </>
  )
}
