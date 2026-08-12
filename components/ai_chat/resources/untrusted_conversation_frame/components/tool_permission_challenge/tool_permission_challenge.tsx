// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import { formatLocale, getLocale } from '$web-common/locale'
import ConversationAreaButton from '../../../common/components/conversation_area_button'
import * as Mojom from '../../../common/mojom'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import { getToolPermissionImplications } from '../assistant_response/get_tool_permission_implications'
import MarkdownRenderer from '../markdown_renderer'
import styles from './tool_permission_challenge.module.scss'

interface Props {
  /**
   * Can we approve or deny the permission challenge?
   */
  isInteractive: boolean

  /**
   * Requires non-null permissionChallenge
   */
  toolUseEvent: Mojom.ToolUseEvent

  /**
   * Display label for the tool that needs permission
   */
  toolLabel: string
}

export default function ToolPermissionChallenge(props: Props) {
  const conversationContext = useUntrustedConversationContext()
  const toolPermissionImplications = getToolPermissionImplications(
    props.toolUseEvent.toolName,
  )

  const permissionChallenge = props.toolUseEvent.permissionChallenge
  if (!permissionChallenge) {
    return null
  }

  // Tools can provide a human-readable, markdown-formatted description of
  // what they want permission to do (e.g. website-provided WebMCP tools
  // describe themselves as "Leo would like to execute **name** on
  // **origin**" instead of the mangled model-facing tool name).
  const summary = permissionChallenge.description ? (
    <MarkdownRenderer
      text={permissionChallenge.description}
      shouldShowTextCursor={false}
    />
  ) : (
    <p>
      {formatLocale(S.CHAT_UI_PERMISSION_CHALLENGE_SUMMARY, {
        $1: <b>{props.toolLabel}</b>,
      })}
    </p>
  )

  return (
    <div className={styles.container}>
      <div className={styles.header}>
        <Icon
          name='warning-triangle-filled'
          className={styles.headerIcon}
        />
        <span className={styles.headerText}>
          {getLocale(S.CHAT_UI_PERMISSION_CHALLENGE_HEADER)}
        </span>
      </div>

      <div className={styles.content}>
        {summary}

        {permissionChallenge.assessment && (
          <>
            <p>{getLocale(S.CHAT_UI_PERMISSION_CHALLENGE_ASSESSMENT_INTRO)}</p>
            <p className={styles.assessment}>
              {permissionChallenge.assessment}
            </p>
          </>
        )}

        {toolPermissionImplications && <p>{toolPermissionImplications}</p>}
        {permissionChallenge.plan && (
          <p className={styles.assessment}>{permissionChallenge.plan}</p>
        )}

        <ConversationAreaButton
          className={styles.permissionButton}
          icon={<>✅</>}
          isDisabled={!props.isInteractive}
          onClick={
            props.isInteractive
              ? () =>
                  conversationContext.conversationHandler?.processPermissionChallenge?.(
                    props.toolUseEvent.id,
                    true,
                  )
              : () => {}
          }
        >
          <div className={styles.permissionButtonText}>
            {getLocale(S.CHAT_UI_PERMISSION_CHALLENGE_ALLOW_BUTTON)}
          </div>
        </ConversationAreaButton>
        <ConversationAreaButton
          className={styles.permissionButton}
          icon={<>❌</>}
          isDisabled={!props.isInteractive}
          onClick={
            props.isInteractive
              ? () =>
                  conversationContext.conversationHandler?.processPermissionChallenge?.(
                    props.toolUseEvent.id,
                    false,
                  )
              : () => {}
          }
        >
          <div className={styles.permissionButtonText}>
            {getLocale(S.CHAT_UI_PERMISSION_CHALLENGE_DENY_BUTTON)}
          </div>
        </ConversationAreaButton>
      </div>
    </div>
  )
}
