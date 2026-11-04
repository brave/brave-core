// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Alert from '@brave/nala/react/alert'
import Button from '@brave/nala/react/button'
import Icon from '@brave/nala/react/icon'
import ProgressRing from '@brave/nala/react/progressRing'
import Toggle from '@brave/nala/react/toggle'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useAIChat } from '../../state/ai_chat_context'
import { ConnectorStatus } from './connector_status'
import { getConnectorErrorMessage, getConnectorOrigin } from './connector_utils'
import styles from './style.module.scss'

interface Props {
  /** Undefined if the connectors couldn't be fetched. */
  servers: Mojom.MCPServer[] | undefined
  isLoading: boolean
  /** An error to show with the list, e.g. after a change failed. */
  error?: string
  onError: (message: string) => void
  onAdd: () => void
  onEdit: (serverId: string) => void
  onRemove: (serverId: string) => void
}

/** The user's connectors, each with its status and settings. */
export default function ConnectorList(props: Props) {
  let list: React.ReactNode = null
  if (props.isLoading) {
    list = (
      <div className={styles.placeholder}>
        <ProgressRing />
      </div>
    )
  } else if (props.servers?.length === 0) {
    list = (
      <div className={styles.placeholder}>
        {getLocale(S.CHAT_UI_CONNECTORS_EMPTY)}
      </div>
    )
  } else if (props.servers) {
    list = (
      <ul className={styles.list}>
        {props.servers.map((server) => (
          <ConnectorRow
            key={server.id}
            server={server}
            onError={props.onError}
            onEdit={() => props.onEdit(server.id)}
            onRemove={() => props.onRemove(server.id)}
          />
        ))}
      </ul>
    )
  }

  return (
    <>
      <div className={styles.body}>
        <div className={styles.description}>
          {getLocale(S.CHAT_UI_CONNECTORS_DESCRIPTION)}
        </div>
        {props.error && <Alert type='error'>{props.error}</Alert>}
        {list}
      </div>
      <div slot='actions'>
        <Button
          kind='filled'
          onClick={props.onAdd}
        >
          <Icon
            slot='icon-before'
            name='plus-add'
          />
          {getLocale(S.CHAT_UI_CONNECTORS_ADD_BUTTON_LABEL)}
        </Button>
      </div>
    </>
  )
}

type Settings = Pick<Mojom.MCPServer, 'enabled' | 'autoAttach'>

function ConnectorRow(props: {
  server: Mojom.MCPServer
  onError: (message: string) => void
  onEdit: () => void
  onRemove: () => void
}) {
  const { api } = useAIChat()
  const { server } = props
  const nameId = React.useId()

  // A change shows until the host reports it, so that the toggle doesn't flip
  // back in the meantime.
  const [pending, setPending] = React.useState<Partial<Settings>>({})
  const enabled = pending.enabled ?? server.enabled
  const autoAttach = pending.autoAttach ?? server.autoAttach

  React.useEffect(() => {
    setPending((current) => {
      const next = { ...current }
      if (next.enabled === server.enabled) {
        delete next.enabled
      }
      if (next.autoAttach === server.autoAttach) {
        delete next.autoAttach
      }
      return next
    })
  }, [server.enabled, server.autoAttach])

  const update = async (change: Partial<Settings>) => {
    setPending((current) => ({ ...current, ...change }))
    try {
      await api.updateServer([
        server.id,
        {
          name: server.name,
          endpoint: server.endpoint,
          enabled,
          autoAttach,
          ...change,
        },
        null,
      ])
    } catch (error) {
      setPending((current) => {
        const reverted = { ...current }
        for (const key of Object.keys(change) as Array<keyof Settings>) {
          delete reverted[key]
        }
        return reverted
      })
      props.onError(getConnectorErrorMessage(error))
    }
  }

  const editLabel = getLocale(S.CHAT_UI_EDIT_BUTTON_LABEL)
  const removeLabel = getLocale(S.CHAT_UI_CONNECTOR_REMOVE_BUTTON_LABEL)

  return (
    <li>
      <div
        role='group'
        aria-labelledby={nameId}
        className={styles.connector}
        data-testid='connector'
      >
        <div className={styles.connectorHeader}>
          <Icon
            name='cloud-hosting'
            className={styles.connectorIcon}
          />
          <div className={styles.connectorDetails}>
            <div
              id={nameId}
              className={styles.connectorName}
            >
              {server.name}
            </div>
            <div className={styles.connectorOrigin}>
              {getConnectorOrigin(server.endpoint.url)}
            </div>
            <ConnectorStatus server={server} />
          </div>
          <div className={styles.connectorActions}>
            <Button
              fab
              kind='plain-faint'
              title={editLabel}
              aria-label={editLabel}
              onClick={props.onEdit}
            >
              <Icon name='edit-pencil' />
            </Button>
            <Button
              fab
              kind='plain-faint'
              title={removeLabel}
              aria-label={removeLabel}
              onClick={props.onRemove}
            >
              <Icon name='trash' />
            </Button>
          </div>
        </div>
        <Toggle
          className={styles.toggleRow}
          checked={enabled}
          onChange={({ checked }) => update({ enabled: checked })}
          data-testid='connector-enabled'
        >
          <span className={styles.toggleLabel}>
            {getLocale(S.CHAT_UI_CONNECTOR_ENABLED_LABEL)}
          </span>
        </Toggle>
        <Toggle
          className={styles.toggleRow}
          checked={autoAttach}
          // Only an enabled connector's tools are offered to a chat.
          disabled={!enabled}
          onChange={({ checked }) => update({ autoAttach: checked })}
          data-testid='connector-auto-attach'
        >
          <span className={styles.toggleLabel}>
            {getLocale(S.CHAT_UI_CONNECTOR_AUTO_ATTACH_LABEL)}
          </span>
        </Toggle>
      </div>
    </li>
  )
}
