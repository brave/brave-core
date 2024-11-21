/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styles from './style.module.scss'
import classnames from '$web-common/classnames'
import Icon from '@brave/leo/react/icon'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import { useAIChat } from '../../state/ai_chat_context'
import { getLocale } from '$web-common/locale'
import getAPI from '../../api'
import { useConversation } from '../../state/conversation_context'
import Alert from '@brave/leo/react/alert'

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

interface DisplayTitleProps {
  title: string
  description?: string
  onEditTitle?: () => void
  onDelete?: () => void
}

function DisplayTitle(props: DisplayTitleProps) {
  const [isButtonMenuVisible, setIsButtonMenuVisible] = React.useState(false)

  return (
    <div
      className={styles.displayTitle}
      onMouseEnter={() => setIsButtonMenuVisible(true)}
      onMouseLeave={() => setIsButtonMenuVisible(false)}
    >
      <div className={styles.displayTitleContent}>
        <div
          className={styles.text}
          onDoubleClick={props.onEditTitle}
          title={props.title}
        >
          {props.title}
        </div>
        <div className={styles.description}>{props.description}</div>
      </div>
      {isButtonMenuVisible && (
        <ButtonMenu className={styles.optionsMenu}>
          <div
            slot='anchor-content'
            className={styles.optionsButton}
          >
            <Icon name='more-vertical' />
          </div>
          <leo-menu-item onClick={props.onEditTitle}>
            <div className={styles.optionsMenuItemWithIcon}>
              <Icon name='edit-pencil' />
              <div>{getLocale('menuRenameConversation')}</div>
            </div>
          </leo-menu-item>
          <leo-menu-item onClick={props.onDelete}>
            <div className={styles.optionsMenuItemWithIcon}>
              <Icon name='trash' />
              <div>{getLocale('menuDeleteConversation')}</div>
            </div>
          </leo-menu-item>
        </ButtonMenu>
      )}
    </div>
  )
}

interface ConversationsListProps {
  setIsConversationsListOpen?: (value: boolean) => unknown
}

export default function ConversationsList(props: ConversationsListProps) {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  return (
    <>
      <div className={styles.scroller}>
        <nav className={styles.nav}>
          {aiChatContext.visibleConversations.length === 0 &&
          <Alert type='info'>
            <Icon name='history' slot='icon' />
            <div slot='title'>{getLocale('noticeConversationHistoryTitle')}</div>
            {getLocale('noticeConversationHistoryEmpty')}
          </Alert>
          }
          {aiChatContext.visibleConversations.length > 0 &&
          <ol>
            {aiChatContext.visibleConversations.map(item => {
              return (
                <li key={item.uuid}>
                  <a
                    className={classnames({
                      [styles.navItem]: true,
                      [styles.navItemActive]: item.uuid === conversationContext.conversationUuid
                    })}
                    onClick={() => {
                      props.setIsConversationsListOpen?.(false)
                    }}
                    href={`/${item.uuid}`}
                  >
                    {item.uuid === aiChatContext.editingConversationId ? (
                      <div className={styles.editibleTitle}>
                        <SimpleInput
                          text={item.title}
                          onBlur={() => aiChatContext.setEditingConversationId(null)}
                          onSubmit={(value) => {
                            aiChatContext.setEditingConversationId(null)
                            getAPI().Service.renameConversation(item.uuid, value)
                          }}
                        />
                      </div>
                    ) : (
                      <DisplayTitle
                        title={item.title || getLocale('conversationListUntitled')}
                        description=''
                        onEditTitle={() => aiChatContext.setEditingConversationId(item.uuid)}
                        onDelete={() => getAPI().Service.deleteConversation(item.uuid)}
                      />
                    )}
                  </a>
                </li>
              )
            })}
          </ol>
          }
        </nav>
      </div>
    </>
  )
}
