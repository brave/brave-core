// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Hooks
import {
  useOnClickOutside
} from '../../../common/hooks/useOnClickOutside'

import {
  DefaultPanelMenu
} from '../wallet-menus/default-panel-menu'

// Styled Components
import {
  Button,
  ButtonIcon
} from './shared-panel-headers.style'
import {
  HeaderTitle,
  MenuWrapper
} from './shared-card-headers.style'
import { Row } from '../../shared/style'

interface Props {
  title: string
}

export const DefaultPanelHeader = (props: Props) => {
  const {
    title
  } = props

  // State
  const [showSettingsMenu, setShowSettingsMenu] =
    React.useState<boolean>(false)

  // Refs
  const settingsMenuRef =
    React.useRef<HTMLDivElement>(null)

  // Hooks
  useOnClickOutside(
    settingsMenuRef,
    () => setShowSettingsMenu(false),
    showSettingsMenu
  )

  // Methods
  const onClickExpand = React.useCallback(() => {
    chrome.tabs.create({ url: 'chrome://wallet/crypto' }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }, [])

  return (
    <Row
      padding='18px 16px'
      justifyContent='space-between'
    >
      <Button
        onClick={onClickExpand}
      >
        <ButtonIcon name='expand' />
      </Button>
      <HeaderTitle
        isPanel={true}
      >
        {title}
      </HeaderTitle>
      <MenuWrapper
        ref={settingsMenuRef}
      >
        <Button
          onClick={
            () => setShowSettingsMenu(prev => !prev)
          }
        >
          <ButtonIcon
            name='more-horizontal'
          />
        </Button>
        {showSettingsMenu &&
          <DefaultPanelMenu />
        }
      </MenuWrapper>
    </Row>
  )
}

export default DefaultPanelHeader
