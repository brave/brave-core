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
import DataContext from '../../state/context'

const conversationList = [
  {
    title: 'Theories of AI',
    description:
      'This is the summary of the conversation, caps at two lines maximum.',
    createdAt: '2024-01-01'
  },
  {
    title: 'Interpersonal Communication',
    description:
      'This is the summary of the conversation, caps at two lines maximum.',
    createdAt: '2024-01-02'
  },
  {
    title: 'Ethical gains and losses with progress in AI',
    description:
      'This is the summary of the conversation, caps at two lines maximum. This is the summary of the conversation, caps at two lines maximum.',
    createdAt: '2024-01-03'
  },
  {
    title:
      'AI and the future of work is promising with the right policies in place',
    description:
      'This is the summary of the conversation, caps at two lines maximum.',
    createdAt: '2024-01-04'
  }
]

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
          <leo-menu-item>
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
}

export default function SidebarNav(props: SidebarNavProps) {
  const context = React.useContext(DataContext)
  const [editingIndex, setEditingIndex] = React.useState<number | null>(null)

  return (
    <>
      <div
        className={classnames({
          [styles.header]: true,
          [styles.noBorder]: !props.enableBackButton
        })}
      >
        {props.enableBackButton && (
          <Button
            kind='plain-faint'
            fab
            onClick={() => context.setIsConversationListOpen(false)}
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
          <div className={styles.timeLabel}>Today</div>
          <ol>
            {conversationList.map((item, index) => {
              return (
                <li>
                  <div
                    className={classnames({
                      [styles.navItem]: true,
                      [styles.navItemActive]: index === 0
                    })}
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
                        title={item.title}
                        description={item.description}
                        onEditTitle={() => setEditingIndex(index)}
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
