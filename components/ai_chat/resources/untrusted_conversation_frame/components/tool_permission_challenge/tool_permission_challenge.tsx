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
import CodeBlock from '../code_block'
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

  /**
   * The tool's arguments, already parsed from `toolUseEvent.argumentsJson` by
   * the parent. It is created by parsing LLM output, so it could be anything -
   * including `null` when the JSON was absent or malformed. In that case we
   * fall back to displaying the raw `argumentsJson`, since the user should
   * still be able to see what the tool was asked to do before allowing it.
   */
  toolInput?: any
}

/**
 * The exact arguments the tool would be called with, for the user to inspect
 * before granting permission. Renders nothing when there is nothing useful to
 * show, so that tools taking no arguments don't get an empty disclosure.
 */
function ToolArguments({
  toolInput,
  argumentsJson,
}: {
  toolInput: any
  argumentsJson: string
}) {
  const [isExpanded, setIsExpanded] = React.useState(false)

  const argumentsText = React.useMemo(() => {
    // Pretty-print whatever the parent managed to parse. Anything that came
    // out of JSON.parse is safe to re-stringify.
    if (typeof toolInput === 'object' && toolInput !== null) {
      // Don't show a disclosure for `{}` - there are no arguments to inspect.
      if (!Array.isArray(toolInput) && Object.keys(toolInput).length === 0) {
        return null
      }
      return JSON.stringify(toolInput, null, 2)
    }
    // Malformed, partial or non-object JSON - show it verbatim rather than
    // hiding the arguments the user is being asked to approve.
    return argumentsJson || null
  }, [toolInput, argumentsJson])

  if (!argumentsText) {
    return null
  }

  return (
    <div>
      <button
        className={styles.argumentsToggle}
        aria-expanded={isExpanded}
        data-testid='tool-arguments-toggle'
        onClick={() => setIsExpanded((expanded) => !expanded)}
      >
        {isExpanded
          ? getLocale(S.CHAT_UI_PERMISSION_CHALLENGE_HIDE_ARGUMENTS_BUTTON)
          : getLocale(S.CHAT_UI_PERMISSION_CHALLENGE_SHOW_ARGUMENTS_BUTTON)}
        <Icon name={isExpanded ? 'carat-down' : 'carat-right'} />
      </button>
      {isExpanded && (
        <div
          className={styles.arguments}
          data-testid='tool-arguments'
        >
          <CodeBlock.Block
            code={argumentsText}
            lang='json'
          />
        </div>
      )}
    </div>
  )
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
  // describe themselves as "Brave AI would like to execute **name** on
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

        <ToolArguments
          toolInput={props.toolInput}
          argumentsJson={props.toolUseEvent.argumentsJson}
        />

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
