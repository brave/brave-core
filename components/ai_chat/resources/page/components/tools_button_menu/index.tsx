// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import classnames from '$web-common/classnames'
import * as mojom from '../../api/page_handler'

import styles from './style.module.scss'
import DataContext from '../../state/context'

interface Props {
  children: React.ReactNode
}

export default function ToolsButtonMenu(props: Props) {
  const context = React.useContext(DataContext)

  return (
    <ButtonMenu
      className={classnames({
        [styles.buttonMenu]: true,
        [styles.highlightFirstItem]:
          context.isToolsMenuOpen && context.inputText.startsWith('/')
      })}
      isOpen={context.isToolsMenuOpen}
      onClose={() => context.setIsToolsMenuOpen(false)}
    >
      <div slot='anchor-content'>{props.children}</div>
      {context.actionList.map((actionGroup) => {
        return (
          <>
            <div className={styles.menuSectionTitle}>
              {actionGroup.category}
            </div>
            {actionGroup.entries.map((entry, i) => {
              if (entry.subheading) {
                return <div className={styles.menuSubtitle}>{entry.subheading}</div>
              } else {
                return (
                  <leo-menu-item
                    key={i}
                    onClick={() =>
                      context.handleActionTypeClick(
                        entry.details?.type as mojom.ActionType
                      )
                    }
                  >
                    {entry.details?.label}
                  </leo-menu-item>
                )
              }
            })}
          </>
        )
      })}
    </ButtonMenu>
  )
}
