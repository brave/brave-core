// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import classnames from '$web-common/classnames'
import ConversationAreaButton from '../../../common/components/conversation_area_button'
import { createTextContentBlock } from '../../../common/content_block'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import type { ToolComponent, ToolUseContent } from './tool_event'
import styles from './tool_event.module.scss'

// Mapping of tool names to human-friendly display names
const TOOL_DISPLAY_NAMES: Record<string, string> = {
  'web_page_navigator': 'Web Navigation',
  'type_text': 'Text Typing',
  'click_element': 'Click Element',
}

// Component that manages state - rendered as JSX to isolate hooks
const SecurityApprovalContent: React.FC<{
  toolUseEvent: any
  isEntryActive: boolean
  context: any
  urlInfo: any
  typeTextInfo: string | null
  reasoning: string
}> = ({ toolUseEvent, isEntryActive, context, urlInfo, typeTextInfo, reasoning }) => {
  const securityWarningRef = React.useRef<HTMLDivElement>(null)
  const [isDetailsExpanded, setIsDetailsExpanded] = React.useState(false)

  // Auto-scroll to security warning when it appears
  React.useEffect(() => {
    if (!toolUseEvent.output && isEntryActive && securityWarningRef.current) {
      const timeoutId = setTimeout(() => {
        if (securityWarningRef.current) {
          securityWarningRef.current.scrollIntoView({ 
            behavior: 'smooth', 
            block: 'center',
            inline: 'nearest'
          })
        }
      }, 100)
      return () => clearTimeout(timeoutId)
    }
    return undefined
  }, [toolUseEvent.output, isEntryActive])

  const handleApproval = (approved: boolean) => {
    if (!isEntryActive) return
    
    const message = approved 
      ? `User approved the tool execution despite security concerns.`
      : `User declined the tool execution due to security concerns.`
    
    context.conversationHandler?.respondToToolUseRequest(
      toolUseEvent.id,
      [createTextContentBlock(message)],
    )
  }

  if (toolUseEvent.output) {
    const outputText = toolUseEvent.output[0]?.textContentBlock?.text || ''
    const isApproved = !outputText.includes('declined')
    
    return (
      <ConversationAreaButton
        isDisabled
        icon={
          <Icon
            className={styles.completedChoiceIcon}
            name={isApproved ? 'checkbox-checked' : 'close-circle'}
          />
        }
      >
        <span data-testid='tool-security-approval-result'>
          {outputText}
        </span>
      </ConversationAreaButton>
    )
  }

  const hasExpandableDetails = typeTextInfo || urlInfo

  return (
    <div className={styles.securityApproval} ref={securityWarningRef}>
      <div 
        className={classnames(
          styles.securityWarning,
          hasExpandableDetails && styles.securityWarningClickable
        )}
        onClick={hasExpandableDetails ? () => setIsDetailsExpanded(!isDetailsExpanded) : undefined}
      >
        <span>Security Warning ({TOOL_DISPLAY_NAMES[toolUseEvent.toolName] || toolUseEvent.toolName})</span>
        {hasExpandableDetails && (
          <Icon name={isDetailsExpanded ? 'carat-up' : 'carat-down'} />
        )}
      </div>
      {hasExpandableDetails && isDetailsExpanded && (
        <div className={styles.securityDetails}>
          {typeTextInfo && (
            <div>
              <div className={styles.securityDetailsLabel}>Leo will type this text:</div>
              <div className={styles.securityDetailsValue}>"{typeTextInfo}"</div>
            </div>
          )}
          {urlInfo && (
            <div>
              <div className={styles.securityDetailsLabel}>Leo will open this URL:</div>
              <div className={styles.securityDetailsValue}>{urlInfo.fullUrl}</div>
            </div>
          )}
        </div>
      )}
      <div className={styles.securityReasoning}>
        {reasoning}
      </div>
      <div className={styles.securityButtons}>
        <ConversationAreaButton
          className={classnames(styles.declineButton)}
          isDisabled={!isEntryActive}
          onClick={() => handleApproval(false)}
          icon={
            <Icon name='close' />
          }
        >
          <span data-testid='tool-security-decline'>Decline</span>
        </ConversationAreaButton>
        <ConversationAreaButton
          className={classnames(styles.approveButton)}
          isDisabled={!isEntryActive}
          onClick={() => handleApproval(true)}
          icon={
            <Icon name='warning-triangle-outline' />
          }
        >
          <span data-testid='tool-security-approve'>
            {urlInfo ? `Proceed to ${urlInfo.displayDomain}` : 'Proceed Anyway'}
          </span>
        </ConversationAreaButton>
      </div>
    </div>
  )
}

const ToolEventContentSecurityApproval: ToolComponent = (props) => {
  const context = useUntrustedConversationContext()
  const content: ToolUseContent = {
    toolLabel: null,
    expandedContent: null,
  }

  // Helper function to extract eTLD+1 from domain
  const getShortDomain = (domain: string): string => {
    const parts = domain.split('.')
    if (parts.length <= 2) return domain
    return parts.slice(-2).join('.')
  }

  // Helper function to extract URL and domain info for web navigation
  const getUrlInfo = () => {
    if (props.toolUseEvent.toolName !== 'web_page_navigator') {
      return null
    }

    try {
      // Try to get from parsed toolInput first, then fall back to raw argumentsJson
      let args
      if (props.toolInput && typeof props.toolInput === 'object') {
        args = props.toolInput
      } else {
        args = JSON.parse(props.toolUseEvent.argumentsJson || '{}')
      }
      
      // Check common field names for URL in web navigation tools
      const urlString = args.website_url
      
      if (!urlString || typeof urlString !== 'string') return null

      const url = new URL(urlString)
      const fullDomain = url.hostname
      const shortDomain = fullDomain.length > 25 ? getShortDomain(fullDomain) : fullDomain
      
      return {
        fullUrl: urlString,
        fullDomain,
        displayDomain: shortDomain
      }
    } catch (e) {
      console.warn('Failed to parse URL from web_page_navigator arguments:', e)
      return null
    }
  }

  // Helper function to extract text that will be typed for type_text tool
  const getTypeTextInfo = () => {
    if (props.toolUseEvent.toolName !== 'type_text') {
      return null
    }

    try {
      // Try to get from parsed toolInput first, then fall back to raw argumentsJson
      let args
      if (props.toolInput && typeof props.toolInput === 'object') {
        args = props.toolInput
      } else {
        args = JSON.parse(props.toolUseEvent.argumentsJson || '{}')
      }

      return args.text || null
    } catch (e) {
      return null
    }
  }

  const urlInfo = getUrlInfo()
  const typeTextInfo = getTypeTextInfo()
  const reasoning = props.toolUseEvent.securityMetadataReasoning || 'This tool call may not be safe to execute.'

  content.expandedContent = (
    <SecurityApprovalContent
      toolUseEvent={props.toolUseEvent}
      isEntryActive={props.isEntryActive}
      context={context}
      urlInfo={urlInfo}
      typeTextInfo={typeTextInfo}
      reasoning={reasoning}
    />
  )

  return props.children(content)
}

export default ToolEventContentSecurityApproval
