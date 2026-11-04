// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/nala/react/button'
import Icon from '@brave/nala/react/icon'
import ProgressRing from '@brave/nala/react/progressRing'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useAIChat } from '../../state/ai_chat_context'
import {
  canReconnectConnector,
  getConnectorErrorMessage,
  getConnectorStatusLabel,
  isConnectorFailing,
} from './connector_utils'
import styles from './style.module.scss'

/**
 * Whether Leo can use a connector, as the browser last found, which every
 * chat it's attached to shows too. A connector that is turned off isn't
 * connected to, so has no status.
 */
export function ConnectorStatus(props: { server: Mojom.MCPServer }) {
  const { api } = useAIChat()
  const { data: statuses, isPlaceholderData } = api.useGetServerStatuses()
  if (props.server.tokenUnreadable) {
    return (
      <ConnectorStatusMessage
        kind='warning'
        text={getConnectorErrorMessage(Mojom.MCPServerError.kTokenUnreadable)}
      />
    )
  }
  if (!props.server.enabled) {
    return null
  }
  const status = statuses?.[props.server.id] ?? Mojom.MCPServerStatus.kAvailable
  // Known first, so that one that's failing isn't asked for its tools.
  if (isPlaceholderData || status === Mojom.MCPServerStatus.kChecking) {
    return <ConnectorConnecting />
  }
  if (isConnectorFailing(status)) {
    return (
      <div className={styles.statusRow}>
        <ConnectorStatusMessage
          kind='warning'
          text={getConnectorStatusLabel(status)}
        />
        {canReconnectConnector(status) && (
          <Button
            size='small'
            kind='plain'
            onClick={() => api.mcpHost.reconnectServer(props.server.id)}
            data-testid='connector-reconnect'
          >
            {getLocale(S.CHAT_UI_CONNECTOR_RECONNECT_BUTTON_LABEL)}
          </Button>
        )}
      </div>
    )
  }
  return <ConnectorToolsStatus serverId={props.server.id} />
}

function ConnectorConnecting() {
  return (
    <div className={styles.status}>
      <ProgressRing />
      {getLocale(S.CHAT_UI_CONNECTOR_STATUS_CONNECTING)}
    </div>
  )
}

function ConnectorToolsStatus(props: { serverId: string }) {
  const { api } = useAIChat()
  const { data: tools } = api.useGetServerTools(props.serverId)
  // A listing that fails changes the connector's status, which is what's
  // shown once it arrives.
  if (!tools) {
    return <ConnectorConnecting />
  }
  return <ConnectorToolCount count={tools.length} />
}

/**
 * How many tools a connector offers: as its status, or as the result of
 * testing the connection to it.
 */
export function ConnectorToolCount(props: {
  count: number
  isTestResult?: boolean
}) {
  const { api } = useAIChat()
  const { data: text } = api.useGetPluralString(
    props.isTestResult
      ? S.CHAT_UI_CONNECTOR_TEST_SUCCESS
      : S.CHAT_UI_CONNECTOR_TOOL_COUNT,
    props.count,
  )
  if (props.isTestResult) {
    return (
      <ConnectorStatusMessage
        kind='success'
        text={text ?? ''}
      />
    )
  }
  return <div className={styles.status}>{text}</div>
}

/**
 * A connector's status, with an icon so that it isn't shown by colour alone.
 */
export function ConnectorStatusMessage(props: {
  kind: 'warning' | 'success'
  text: string
}) {
  return (
    <div
      className={classnames(styles.status, {
        [styles.statusWarning]: props.kind === 'warning',
        [styles.statusSuccess]: props.kind === 'success',
      })}
    >
      <Icon
        name={
          props.kind === 'warning'
            ? 'warning-triangle-outline'
            : 'check-circle-outline'
        }
      />
      {props.text}
    </div>
  )
}
