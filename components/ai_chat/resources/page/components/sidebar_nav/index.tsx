/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styles from './style.module.scss'
import classnames from '$web-common/classnames'
import Icon from '@brave/leo/react/icon'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Button from '@brave/leo/react/button'
import { useAIChat } from '../../state/ai_chat_context'
import { getLocale } from '$web-common/locale'
import getAPI from '../../api'

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
            <div className={styles.optionsMenuItmWithIcon}>
              <Icon name='edit-pencil' />
              <div>Rename</div>
            </div>
          </leo-menu-item>
          <leo-menu-item onClick={props.onDelete}>
            <div className={styles.optionsMenuItmWithIcon}>
              <Icon name='trash' />
              <div>Delete</div>
            </div>
          </leo-menu-item>
        </ButtonMenu>
      )}
    </div>
  )
}

interface SidebarNavProps {
  enableBackButton?: boolean
  setIsConversationListOpen: (value: boolean) => unknown
}

export default function SidebarNav(props: SidebarNavProps) {
  const aiChatContext = useAIChat()
  const [editingIndex, setEditingIndex] = React.useState<number | null>(null)

  return (
    <>
      <div
        className={classnames({
          [styles.header]: true,
          [styles.noBorder]: !props.enableBackButton
        })}
      >
        {!aiChatContext.isStandalone && props.enableBackButton && (
          <Button
            kind='plain-faint'
            fab
            onClick={() => {
              props.setIsConversationListOpen?.(false)
            }}
          >
            <Icon name='arrow-left' />
          </Button>
        )}
        <form className={styles.form}>
          <input
            placeholder='Search your conversations'
            type='text'
          />
        </form>
      </div>
      <div className={styles.scroller}>
        <nav className={styles.nav}>
          {/* <div className={styles.timeLabel}>Today</div> */}
          <ol>
            {aiChatContext.visibleConversations.map((item, index) => {
              return (
                <li key={item.uuid}>
                  <div
                    className={classnames({
                      [styles.navItem]: true,
                      [styles.navItemActive]: item.uuid === aiChatContext.selectedConversationId
                    })}
                    onClick={() => {
                      aiChatContext.onSelectConversationId(item.uuid)
                      props.setIsConversationListOpen?.(false)
                    }}
                  >
                    {editingIndex === index ? (
                      <div className={styles.editibleTitle}>
                        <SimpleInput
                          text={item.title}
                          onBlur={() => setEditingIndex(null)}
                          onSubmit={(value) => {
                            console.log(value)
                            setEditingIndex(null)
                          }}
                        />
                      </div>
                    ) : (
                      <DisplayTitle
                        title={item.title ? item.title : item.summary ? item.summary : getLocale('conversationListUntitled')}
                        description={''}
                        onEditTitle={() => setEditingIndex(index)}
                        onDelete={() => getAPI().Service.deleteConversation(item.uuid)}
                      />
                    )}
                  </div>
                </li>
              )
            })}
          </ol>
        </nav>
      </div>
    </>
  )
}
