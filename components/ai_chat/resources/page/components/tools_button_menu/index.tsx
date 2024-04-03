// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import classnames from 'classnames'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import styles from './style.module.scss'
import DataContext from '../../state/context'
import * as mojom from '../../api/page_handler'

const data = [
  {
    category: 'Quick actions',
    actions: [
      { label: 'Explain', type: mojom.ActionType.EXPLAIN },
      { label: 'Paraphrase', type: mojom.ActionType.PARAPHRASE },
      { label: 'Improve', type: mojom.ActionType.IMPROVE }
    ]
  }
]

export default function ToolsButtonMenu() {
  const context = React.useContext(DataContext)

  return (
    <ButtonMenu
      isOpen={context.isToolsMenuOpen}
      onClose={() => context.setIsToolsMenuOpen(false)}
    >
      <Button
        slot='anchor-content'
        size='small'
        kind='plain-faint'
        onClick={() => context.setIsToolsMenuOpen(!context.isToolsMenuOpen)}
      >
        <div
          className={classnames({
            [styles.slashIcon]: true,
            [styles.slashIconActive]: context.isToolsMenuOpen
          })}
        >
          <Icon name='slash' />
        </div>
      </Button>
      {data.map((entry) => {
        return (
          <>
            <div className={styles.menuSectionTitle}>{entry.category}</div>
            {entry.actions.map((action) => (
              <leo-menu-item
                onClick={() => context.handleActionTypeClick(action.type)}
              >
                {action.label}
              </leo-menu-item>
            ))}
          </>
        )
      })}
    </ButtonMenu>
  )
}
