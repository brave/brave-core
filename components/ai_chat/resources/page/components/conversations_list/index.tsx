/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styles from './style.module.scss'
import classnames from '$web-common/classnames'
import Icon from '@brave/leo/react/icon'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import * as Mojom from '../../../common/mojom'
import { useAIChat } from '../../state/ai_chat_context'
import { getLocale } from '$web-common/locale'
import getAPI from '../../api'
import { useConversation } from '../../state/conversation_context'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import { Link } from '$web-common/useRoute'

interface SimpleInputProps {
  text?: string
  onSubmit?: (value: string) => void
  onBlur?: () => void
}

function SimpleInput(props: SimpleInputProps) {
  const [value, setValue] = React.useState(props.text || '')

  const handleChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setValue(event.target.value)
  }

  const handleSubmit = (event: React.FormEvent<HTMLFormElement>) => {
    event.preventDefault()
    props.onSubmit?.(value)
  }

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Escape') {
      props.onBlur?.()
    }
  }

  return (
    <form onSubmit={handleSubmit}>
      <input
        className={styles.simpleInput}
        type='text'
        value={value}
        onChange={handleChange}
        autoFocus
        onBlur={props.onBlur}
        onKeyDown={handleKeyDown}
      />
    </form>
  )
}

interface ConversationItemProps extends ConversationsListProps {
  conversation: Mojom.Conversation
}

function ConversationItem(props: ConversationItemProps) {
  const [isOptionsMenuOpen, setIsOptionsMenuOpen] = React.useState(false)

  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  const { uuid } = props.conversation
  const title =
    props.conversation.title || getLocale(S.AI_CHAT_CONVERSATION_LIST_UNTITLED)

  const handleButtonMenuChange = (e: { isOpen: boolean }) => {
    setIsOptionsMenuOpen(e.isOpen)
  }

  const handleEditTitle: EventListener = (e) => {
    e.preventDefault()
    aiChatContext.setEditingConversationId(uuid)
  }

  const handleDelete: EventListener = (e) => {
    e.preventDefault()
    aiChatContext.setDeletingConversationId(uuid)
  }

  const isEditing = aiChatContext.editingConversationId === uuid
  const isActive = uuid === conversationContext.conversationUuid

  return (
    <li>
      <Link
        className={classnames(
          styles.navItem,
          isActive && styles.navItemActive,
          isOptionsMenuOpen && styles.isOptionsMenuOpen,
        )}
        onClick={(e) => {
          if (isEditing) {
            e.preventDefault()
            return
          }
          props.setIsConversationsListOpen?.(false)
        }}
        onDoubleClick={() => aiChatContext.setEditingConversationId(uuid)}
        href={`/${uuid}`}
      >
        <div className={styles.displayTitle}>
          <div className={styles.displayTitleContent}>
            <div
              className={styles.text}
              title={title}
            >
              {title}
            </div>
          </div>
          <ButtonMenu
            className={styles.optionsMenu}
            onChange={handleButtonMenuChange}
          >
            <Button
              slot='anchor-content'
              className={styles.optionsButton}
              kind='plain-faint'
              fab
              size='small'
            >
              <Icon name='more-vertical' />
            </Button>
            <leo-menu-item onClick={handleEditTitle}>
              <div className={styles.optionsMenuItemWithIcon}>
                <Icon name='edit-pencil' />
                <div>{getLocale(S.CHAT_UI_MENU_RENAME_CONVERSATION)}</div>
              </div>
            </leo-menu-item>
            <leo-menu-item onClick={handleDelete}>
              <div className={styles.optionsMenuItemWithIcon}>
                <Icon name='trash' />
                <div>{getLocale(S.CHAT_UI_MENU_DELETE_CONVERSATION)}</div>
              </div>
            </leo-menu-item>
          </ButtonMenu>
        </div>
        {uuid === aiChatContext.editingConversationId && (
          <div className={styles.editibleTitle}>
            <SimpleInput
              text={title}
              onBlur={() => aiChatContext.setEditingConversationId(null)}
              onSubmit={(value) => {
                aiChatContext.setEditingConversationId(null)
                getAPI().service.renameConversation(uuid, value)
              }}
            />
          </div>
        )}
      </Link>
    </li>
  )
}

interface ConversationsListProps {
  setIsConversationsListOpen?: (value: boolean) => unknown
}

export default function ConversationsList(props: ConversationsListProps) {
  const aiChatContext = useAIChat()
  const startedNonTemporaryConversations = aiChatContext.conversations.filter(
    (c) => !c.temporary && c.hasContent,
  )

  return (
    <>
      <div className={styles.scroller}>
        <nav className={styles.nav}>
          {!aiChatContext.isStoragePrefEnabled && (
            <Alert type='notice'>
              <Icon
                name='history'
                slot='icon'
              />
              <div slot='title'>
                {getLocale(
                  S.CHAT_UI_NOTICE_CONVERSATION_HISTORY_TITLE_DISABLED_PREF,
                )}
              </div>
              {getLocale(S.CHAT_UI_NOTICE_CONVERSATION_HISTORY_DISABLED_PREF)}
              <div slot='actions'>
                <Button
                  kind='outline'
                  onClick={aiChatContext.enableStoragePref}
                >
                  {getLocale(
                    S.CHAT_UI_NOTICE_CONVERSATION_HISTORY_DISABLED_PREF_BUTTON,
                  )}
                </Button>
              </div>
            </Alert>
          )}
          {aiChatContext.isStoragePrefEnabled
            && startedNonTemporaryConversations.length === 0 && (
              <Alert type='notice'>
                <Icon
                  name='history'
                  slot='icon'
                />
                <div slot='title'>
                  {getLocale(S.CHAT_UI_MENU_CONVERSATION_HISTORY)}
                </div>
                {getLocale(S.CHAT_UI_NOTICE_CONVERSATION_HISTORY_EMPTY)}
              </Alert>
            )}
          {startedNonTemporaryConversations.length > 0 && (
            <ol>
              {startedNonTemporaryConversations.map((conversation) => (
                <ConversationItem
                  key={conversation.uuid}
                  {...props}
                  conversation={conversation}
                />
              ))}
            </ol>
          )}
        </nav>
      </div>
    </>
  )
}
