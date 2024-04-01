// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import classnames from 'classnames'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'

import styles from './style.module.scss'
import DataContext from '../../state/context'
import * as mojom from '../../api/page_handler'

export default function ToolsButtonMenu() {
  const context = React.useContext(DataContext)
  const [isMenuOpen, setIsMenuClose] = React.useState(false)

  return (
    <ButtonMenu
      isOpen={isMenuOpen}
      onClose={() => setIsMenuClose(false)}
    >
      <Button
        slot='anchor-content'
        size='small'
        kind='plain-faint'
        onClick={() => setIsMenuClose(!isMenuOpen)}
      >
        <div className={classnames({
          [styles.slashIcon]: true,
          [styles.slashIconActive]: isMenuOpen
        })}>
          <Icon name='slash' />
        </div>
      </Button>
      <div className={styles.menuSectionTitle}>Quick actions</div>
      <leo-menu-item
        onClick={() =>
          context.setUserActionType(
            mojom.ActionType.CREATE_SOCIAL_MEDIA_COMMENT_LONG
          )
        }
      >
        {getLocale('actionsSummarize')}
      </leo-menu-item>
      <leo-menu-item>{getLocale('actionsExplain')}</leo-menu-item>
      <leo-menu-item>{getLocale('actionsParaphrase')}</leo-menu-item>
      <leo-menu-item>{getLocale('actionsImprove')}</leo-menu-item>
    </ButtonMenu>
  )
}
