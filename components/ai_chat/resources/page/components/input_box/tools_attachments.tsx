/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/nala/react/button'
import Icon from '@brave/nala/react/icon'
import ProgressRing from '@brave/nala/react/progressRing'
import Tooltip from '@brave/nala/react/tooltip'
import classnames from '$web-common/classnames'
import { formatLocale, getLocale } from '$web-common/locale'
import {
  isMCPServerContent,
  isWorkspaceContent,
} from '../../../common/conversation_history_utils'
import * as Mojom from '../../../common/mojom'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import {
  canReconnectConnector,
  getConnectorStatusDescription,
  getConnectorStatusLabel,
  isConnectorFailing,
} from '../connectors_modal/connector_utils'
import WebsiteToolsModal from '../website_tools_modal'
import styles from './tools_attachments.module.scss'

const WEBSITE_TOOLS_LEARN_MORE_URL =
  'https://support.brave.app/hc/en-us/articles/50132604817165'

interface Props {
  toolsContent: Mojom.AssociatedContent[]
  setToolsAttached: (
    content: Mojom.AssociatedContent,
    toolsAttached: boolean,
  ) => void
  disassociateContent: (content: Mojom.AssociatedContent) => void
}

function ToolPill(props: {
  content: Mojom.AssociatedContent
  onOpen: () => void
  onRemove: () => void
}) {
  const conversation = useConversation()
  const { getContentToolsData: tools } = conversation.api.useGetContentTools(
    props.content.uuid,
  )
  // Workspace tools have no per-tool permissions to manage, so the pill
  // doesn't open the tools dialog.
  const isWorkspace = isWorkspaceContent(props.content)

  const pillContent = (
    <>
      {isMCPServerContent(props.content) ? (
        <Icon
          className={styles.toolPillIcon}
          name='cloud-hosting'
        />
      ) : (
        <img
          className={styles.toolPillFavicon}
          src={`//favicon2?size=64&pageUrl=${encodeURIComponent(props.content.url.url)}&allowGoogleServerFallback=0`}
        />
      )}
      <span className={styles.toolPillTitle}>{props.content.title}</span>
      {tools.length > 0 && (
        <span
          className={styles.toolPillCount}
          role='img'
          aria-label={formatLocale(S.CHAT_UI_WEBSITE_TOOLS_LIST_LABEL, {
            $1: tools.length.toString(),
          })}
        >
          <Icon name='tool-box' />
          {tools.length}
        </span>
      )}
    </>
  )

  return (
    <div
      className={classnames(styles.toolPill, {
        [styles.toolPillStatic]: isWorkspace,
      })}
    >
      <Tooltip
        mode='mini'
        className={styles.toolPillTitleTooltip}
      >
        {isWorkspace ? (
          <div className={styles.toolPillContent}>{pillContent}</div>
        ) : (
          <button
            type='button'
            className={classnames(styles.toolPillContent, styles.toolPillOpen)}
            onClick={props.onOpen}
          >
            {pillContent}
          </button>
        )}
        <div slot='content'>{props.content.title}</div>
      </Tooltip>
      {props.content.mcpServer && (
        <ConnectorStatus server={props.content.mcpServer} />
      )}
      <Button
        fab
        size='tiny'
        kind='plain-faint'
        className={styles.toolPillRemove}
        title={getLocale(S.CHAT_UI_TOOLS_ATTACHMENT_REMOVE_LABEL)}
        onClick={props.onRemove}
      >
        <Icon name='close' />
      </Button>
    </div>
  )
}

/**
 * Whether Leo can use an attached connector's tools, shown while it's being
 * checked or while it can't, before and after a message is sent.
 */
function ConnectorStatus(props: { server: Mojom.AssociatedMCPServer }) {
  const { api } = useAIChat()
  const descriptionId = React.useId()
  const { serverId, status } = props.server

  if (status === Mojom.MCPServerStatus.kChecking) {
    return (
      <span
        className={styles.toolPillStatus}
        data-testid='connector-status'
      >
        <ProgressRing />
        {getConnectorStatusLabel(status)}
      </span>
    )
  }
  if (!isConnectorFailing(status)) {
    return null
  }

  const reconnectLabel = getLocale(S.CHAT_UI_CONNECTOR_RECONNECT_BUTTON_LABEL)
  return (
    <>
      <Tooltip
        mode='mini'
        positionStrategy='fixed'
        className={styles.toolPillStatusTooltip}
      >
        {/* Focusable, so that why is shown to keyboard and touch too. */}
        <div
          tabIndex={0}
          aria-describedby={descriptionId}
          className={classnames(
            styles.toolPillStatus,
            styles.toolPillStatusWarning,
          )}
          data-testid='connector-status'
        >
          <Icon name='warning-triangle-outline' />
          {getConnectorStatusLabel(status)}
        </div>
        <div
          id={descriptionId}
          slot='content'
          className={styles.toolsTooltipContent}
        >
          {getConnectorStatusDescription(status)}
        </div>
      </Tooltip>
      {canReconnectConnector(status) && (
        <Button
          fab
          size='tiny'
          kind='plain-faint'
          className={styles.toolPillAction}
          title={reconnectLabel}
          aria-label={reconnectLabel}
          onClick={() => api.mcpHost.reconnectServer(serverId)}
          data-testid='connector-reconnect'
        >
          <Icon name='refresh' />
        </Button>
      )}
    </>
  )
}

// A row of pills, one per attached content that provides tools, shown above the
// input. The "Tools" label stays pinned while the pills scroll horizontally.
export default function ToolsAttachments(props: Props) {
  const [openContentUuid, setOpenContentUuid] = React.useState<string | null>(
    null,
  )

  if (props.toolsContent.length === 0) {
    return null
  }

  const openContent = props.toolsContent.find(
    (content) => content.uuid === openContentUuid,
  )

  return (
    <div className={styles.toolsAttachments}>
      <div className={styles.toolsLabel}>
        <span>{getLocale(S.CHAT_UI_TOOLS_ATTACHMENT_LABEL)}</span>
        <Tooltip
          mode='default'
          positionStrategy='fixed'
        >
          <Icon name='info-outline' />
          <div
            slot='content'
            className={styles.toolsTooltipContent}
          >
            <p>{getLocale(S.CHAT_UI_TOOLS_ATTACHMENT_TOOLTIP_INFO)}</p>
            <a
              href={WEBSITE_TOOLS_LEARN_MORE_URL}
              target='_blank'
              className={styles.learnMoreLink}
            >
              {getLocale(S.CHAT_UI_LEARN_MORE)}
            </a>
          </div>
        </Tooltip>
      </div>
      <div className={styles.toolsPills}>
        {props.toolsContent.map((content) => (
          <ToolPill
            key={content.uuid}
            content={content}
            onOpen={() => setOpenContentUuid(content.uuid)}
            // An MCP server is only there for its tools, so is detached
            // with them.
            onRemove={() =>
              isMCPServerContent(content)
                ? props.disassociateContent(content)
                : props.setToolsAttached(content, false)
            }
          />
        ))}
      </div>
      {/* Mounted only while open so the tool list is fetched fresh each time. */}
      {openContent && (
        <WebsiteToolsModal
          content={openContent}
          onClose={() => setOpenContentUuid(null)}
        />
      )}
    </div>
  )
}
