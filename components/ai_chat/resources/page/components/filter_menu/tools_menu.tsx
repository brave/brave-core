// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import {
  ActionEntry,
  ActionType,
  SmartMode,
} from 'components/ai_chat/resources/common/mojom'
import FilterMenu, { Props } from './filter_menu'
import { matches } from './query'
import styles from './style.module.scss'
import { getLocale } from '$web-common/locale'
import { useAIChat } from '../../state/ai_chat_context'

type ToolsMenuProps = {
  handleClick: (type: ActionType) => void
  handleSmartModeClick?: (smartMode: SmartMode) => void
  handleSmartModeEdit?: (smartMode: SmartMode) => void
} & Pick<
  Props<ExtendedActionEntry>,
  'categories' | 'isOpen' | 'setIsOpen' | 'query'
>

// Extended entry type that can include smart modes before we migrate
// existing slash tools (actions) to smart modes.
type ExtendedActionEntry = ActionEntry & {
  smartMode?: SmartMode
}

function matchesQuery(query: string, entry: ExtendedActionEntry) {
  if (entry.details) {
    return matches(query, entry.details.label)
  } else if (entry.smartMode) {
    return matches(query, entry.smartMode.shortcut)
  }
  return false
}

export default function ToolsMenu(props: ToolsMenuProps) {
  const { smartModes } = useAIChat()

  // Create combined categories including smart modes
  const combinedCategories = React.useMemo(() => {
    const categories = []

    if (
      smartModes.length > 0
      && props.handleSmartModeClick
      && props.handleSmartModeEdit
    ) {
      const smartModeEntries: ExtendedActionEntry[] = smartModes.map(
        (mode: SmartMode) => ({
          smartMode: mode,
          subheading: undefined,
          details: undefined,
        }),
      )

      categories.push({
        category: getLocale(S.CHAT_UI_SMART_MODES_GROUP),
        entries: smartModeEntries,
      })
    }

    categories.push(...props.categories)

    return categories
  }, [smartModes])

  return (
    <FilterMenu
      categories={combinedCategories}
      isOpen={props.isOpen}
      setIsOpen={props.setIsOpen}
      query={props.query}
      matchesQuery={matchesQuery}
    >
      {(item: ExtendedActionEntry) =>
        item.subheading ? (
          <div className={styles.menuSubtitle}>{item.subheading}</div>
        ) : item.details ? (
          <leo-menu-item
            key={item.details.type}
            onClick={() => props.handleClick(item.details!.type)}
          >
            {item.details.label}
          </leo-menu-item>
        ) : item.smartMode ? (
          <leo-menu-item
            key={item.smartMode.id}
            onClick={() => props.handleSmartModeClick?.(item.smartMode!)}
          >
            <div className={styles.smartModeWithEdit}>
              <span className={styles.shortcut}>{item.smartMode.shortcut}</span>
              <Button
                fab
                kind='plain-faint'
                className={styles.editIcon}
                onClick={(e) => {
                  props.handleSmartModeEdit?.(item.smartMode!)
                }}
              >
                <Icon name='edit-pencil' />
              </Button>
            </div>
          </leo-menu-item>
        ) : null
      }
    </FilterMenu>
  )
}
