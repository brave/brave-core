// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import classnames from '$web-common/classnames'
import * as Mojom from '../../../common/mojom'
import styles from './style.module.scss'

interface Props {
  inputText: string,
  isToolsMenuOpen: boolean,
  setIsToolsMenuOpen: (open: boolean) => void,
  actionList: Mojom.ActionGroup[],
  handleActionTypeClick: (action: Mojom.ActionType) => void
}

export default function ToolsButtonMenu(props: Props) {
  return (
    <ButtonMenu
      className={classnames({
        [styles.buttonMenu]: true,
        [styles.highlightFirstItem]:
          props.isToolsMenuOpen && props.inputText.startsWith('/')
      })}
      isOpen={props.isToolsMenuOpen}
      onClose={() => props.setIsToolsMenuOpen(false)}
    >
      {props.actionList.map((actionGroup) => {
        return (
          <React.Fragment key={actionGroup.category}>
            <div className={styles.menuSectionTitle}>
              {actionGroup.category}
            </div>
            {actionGroup.entries.map((entry, i) => {
              if (entry.subheading) {
                return <div key={i} className={styles.menuSubtitle}>{entry.subheading}</div>
              } else {
                return (
                  <leo-menu-item
                    key={i}
                    onClick={() =>
                      props.handleActionTypeClick(
                        entry.details?.type ?? Mojom.ActionType.UNSPECIFIED
                      )
                    }
                  >
                    {entry.details?.label}
                  </leo-menu-item>
                )
              }
            })}
          </React.Fragment>
        )
      })}
    </ButtonMenu>
  )
}
