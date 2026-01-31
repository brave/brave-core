// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { ActionEntry, Skill } from 'components/ai_chat/resources/common/mojom'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import FilterMenu, { MatchedText, Props } from './filter_menu'
import { matches } from './query'
import styles from './style.module.scss'
import { FuzzyFinder } from './fuzzy_finder'

export type ExtendedActionEntry = ActionEntry | Skill

type ToolsMenuProps = {
  isMobile: boolean
  handleClick: (type: ExtendedActionEntry) => void
  handleEditClick: (skill: Skill) => void
  handleNewSkillClick: () => void
  handleAutoSelect?: (entry: ExtendedActionEntry) => void
} & Pick<
  Props<ExtendedActionEntry>,
  'categories' | 'isOpen' | 'setIsOpen' | 'query'
>

const getIsActionEntry = (item: ExtendedActionEntry): item is ActionEntry => {
  return 'details' in item && item.details !== undefined
}

export const getIsSkill = (item: ExtendedActionEntry): item is Skill => {
  return 'shortcut' in item
}

function matchesQuery(query: FuzzyFinder, entry: ExtendedActionEntry) {
  if (getIsActionEntry(entry)) {
    return matches(query, entry.details!.label)
  }
  if (getIsSkill(entry)) {
    return matches(query, entry.shortcut)
  }
  return undefined
}

export default function ToolsMenu(props: ToolsMenuProps) {
  const isAutoSelectRef = React.useRef(false)

  function handleClick(item: ExtendedActionEntry) {
    if (isAutoSelectRef.current) {
      isAutoSelectRef.current = false
      if (props.handleAutoSelect) {
        props.handleAutoSelect(item)
        return
      }
    }
    props.handleClick(item)
  }

  return (
    <FilterMenu
      categories={props.categories}
      isOpen={props.isOpen}
      setIsOpen={props.setIsOpen}
      query={props.query}
      matchesQuery={matchesQuery}
      onAutoSelect={() => (isAutoSelectRef.current = true)}
      noMatchesMessage={
        <div className={styles.toolsNoMatches}>
          {getLocale(S.CHAT_UI_TOOLS_MENU_NO_SKILLS_FOUND)}
        </div>
      }
      footer={
        <div className={styles.toolsMenuFooter}>
          <leo-menu-item
            aria-selected={false}
            onClick={props.handleNewSkillClick}
          >
            <Icon name='plus-add' />
            {getLocale(S.CHAT_UI_TOOLS_MENU_NEW_SKILL_BUTTON_LABEL)}
          </leo-menu-item>
        </div>
      }
    >
      {(item, category, match) => {
        if ('subheading' in item && item.subheading !== undefined) {
          return <div className={styles.menuSubtitle}>{item.subheading}</div>
        }

        const isActionEntry = getIsActionEntry(item)
        return (
          <leo-menu-item
            key={isActionEntry ? item.details!.type : item.shortcut}
            onClick={() => handleClick(item)}
          >
            <div className={styles.toolsMenuItemContent}>
              <MatchedText
                text={isActionEntry ? item.details!.label : item.shortcut}
                match={match}
              />
              {getIsSkill(item) && (
                <Button
                  fab
                  kind='plain-faint'
                  className={classnames({
                    [styles.editButton]: true,
                    [styles.editButtonMobile]: props.isMobile,
                  })}
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
