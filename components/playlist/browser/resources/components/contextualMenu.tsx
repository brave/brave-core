// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled, { css } from 'styled-components'

import Icon from '@brave/leo/react/icon'
import ButtonMenu, { ButtonMenuProps } from '@brave/leo/react/buttonMenu'
import { color, spacing } from '@brave/leo/tokens/css'

interface MenuItemProps {
  name: string
  iconName: string
  onClick: () => void
}

interface MenuProps {
  items: Array<MenuItemProps | undefined>
  visible: boolean
  onShowMenu?: () => void
  onDismissMenu?: () => void
}

const StyledRow = styled.div`
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: ${spacing.m};
`

const StyledButtonMenu = styled(ButtonMenu)<{ visible: boolean }>`
  ${(p) =>
    !p.visible &&
    css`
      visibility: hidden;
    `}
  --leo-icon-size: 20px;
  color: ${color.text.secondary};
`

export default function ContextualMenuAnchorButton ({
  items,
  visible,
  onShowMenu,
  onDismissMenu
}: MenuProps) {
  const menuButtonProps: ButtonMenuProps = {}
  // Force menu widget to be closed when the anchor button is not visible. In
  // case where it's visible, the menuButtonProps doesn't contain isOpen property,
  // so it won't affect the behavior of the menu.
  if (!visible) menuButtonProps.isOpen = false

  // TODO(sko) We don't have event for opening menu widget. Once it's ready,
  // wire onShowMenu and onDismissMenu to corresponding events.
  return (
    <StyledButtonMenu
      tabIndex={0}
      visible={visible}
      {...menuButtonProps}
    >
      <div slot='anchor-content'>
        <Icon name='more-horizontal' />
      </div>
      {items
        .filter((i) => i)
        .map((i) => (
          <leo-menu-item
            key={i!.name}
            onClick={(e) => {
              e.stopPropagation()
              i!.onClick()
            }}
          >
            <StyledRow>
              <span>{i!.name}</span>
              <Icon name={i!.iconName} />
            </StyledRow>
          </leo-menu-item>
        ))}
    </StyledButtonMenu>
  )
}
