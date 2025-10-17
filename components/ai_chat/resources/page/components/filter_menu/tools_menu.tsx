// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import {
  ActionEntry,
  SmartMode,
} from 'components/ai_chat/resources/common/mojom'
import { getLocale } from '$web-common/locale'
import FilterMenu, { Props } from './filter_menu'
import { matches } from './query'
import styles from './style.module.scss'

export type ExtendedActionEntry = ActionEntry | SmartMode

type ToolsMenuProps = {
  handleClick: (type: ExtendedActionEntry) => void
  handleEditClick: (smartMode: SmartMode) => void
  handleNewSmartModeClick: () => void
} & Pick<
  Props<ExtendedActionEntry>,
  'categories' | 'isOpen' | 'setIsOpen' | 'query'
>

const getIsActionEntry = (item: ExtendedActionEntry): item is ActionEntry => {
  return 'details' in item && item.details !== undefined
}

export const getIsSmartMode = (
  item: ExtendedActionEntry,
): item is SmartMode => {
  return 'shortcut' in item
}

function matchesQuery(query: string, entry: ExtendedActionEntry) {
  if (getIsActionEntry(entry)) {
    return matches(query, entry.details!.label)
  }
  if (getIsSmartMode(entry)) {
    return matches(query, entry.shortcut)
  }
  return false
}

export default function ToolsMenu(props: ToolsMenuProps) {
  return (
    <FilterMenu
      categories={props.categories}
      isOpen={props.isOpen}
      setIsOpen={props.setIsOpen}
      query={props.query}
      matchesQuery={matchesQuery}
      noMatchesMessage={
        <div className={styles.toolsNoMatches}>
          {getLocale(S.CHAT_UI_TOOLS_MENU_NO_SMART_MODES_FOUND)}
        </div>
      }
      footer={
        <div className={styles.toolsMenuFooter}>
          <leo-menu-item
            aria-selected={false}
            onClick={props.handleNewSmartModeClick}
          >
            <Icon name='plus-add' />
            {getLocale(S.CHAT_UI_TOOLS_MENU_NEW_SMART_MODE_BUTTON_LABEL)}
          </leo-menu-item>
        </div>
      }
    >
      {(item) => {
        if ('subheading' in item && item.subheading !== undefined) {
          return <div className={styles.menuSubtitle}>{item.subheading}</div>
        }

        const isActionEntry = getIsActionEntry(item)
        return (
          <leo-menu-item
            key={isActionEntry ? item.details!.type : item.shortcut}
            onClick={() => props.handleClick(item)}
          >
            <div className={styles.toolsMenuItemContent}>
              {isActionEntry ? item.details!.label : item.shortcut}
              {getIsSmartMode(item) && (
                <Button
                  fab
                  kind='plain-faint'
                  className={styles.editButton}
                  onClick={(e) => {
                    e.stopPropagation()
                    props.handleEditClick(item)
                  }}
                >
                  <Icon name='edit-pencil' />
                </Button>
              )}
            </div>
          </leo-menu-item>
        )
      }}
    </FilterMenu>
  )
}
