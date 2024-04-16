// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'

import styles from './style.module.scss'
import DataContext from '../../state/context'
import * as mojom from '../../api/page_handler'

const data = [
  {
    category: 'Quick actions',
    actions: [{ label: 'Explain', type: mojom.ActionType.EXPLAIN }]
  },
  {
    category: 'Rewrite',
    actions: [
      { label: 'Paraphrase', type: mojom.ActionType.PARAPHRASE },
      { label: 'Improve', type: mojom.ActionType.IMPROVE },
      { label: 'Change tone', type: -1 },
      { label: 'Change tone / Academic', type: mojom.ActionType.ACADEMICIZE },
      {
        label: 'Change tone / Professional',
        type: mojom.ActionType.PROFESSIONALIZE
      },
      {
        label: 'Change tone / Persuasive',
        type: mojom.ActionType.PERSUASIVE_TONE
      },
      { label: 'Change tone / Casual', type: mojom.ActionType.CASUALIZE },
      { label: 'Change tone / Funny', type: mojom.ActionType.FUNNY_TONE },
      { label: 'Change length / Short', type: mojom.ActionType.SHORTEN },
      { label: 'Change length / Expand', type: mojom.ActionType.EXPAND }
    ]
  },
  {
    category: 'Create',
    actions: [
      { label: 'Tagline', type: mojom.ActionType.CREATE_TAGLINE },
      { label: 'Social media', type: -1 },
      {
        label: 'Social media / Short',
        type: mojom.ActionType.CREATE_SOCIAL_MEDIA_COMMENT_SHORT
      },
      {
        label: 'Social media / Long',
        type: mojom.ActionType.CREATE_SOCIAL_MEDIA_COMMENT_LONG
      }
    ]
  }
]

interface Props {
  children: React.ReactNode
}

export default function ToolsButtonMenu(props: Props) {
  const context = React.useContext(DataContext)

  return (
    <ButtonMenu
      isOpen={context.isToolsMenuOpen}
      onClose={() => context.setIsToolsMenuOpen(false)}
    >
      <div slot='anchor-content'>{props.children}</div>
      {data.map((entry) => {
        return (
          <>
            <div className={styles.menuSectionTitle}>{entry.category}</div>
            {entry.actions.map((action) => {
              if (action.type === -1) {
                return (
                  <div className={styles.menuSubtitle}>{action.label}</div>
                )
              } else {
                return (
                  <leo-menu-item
                    onClick={() => context.handleActionTypeClick(action.type)}
                  >
                    {action.label}
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
