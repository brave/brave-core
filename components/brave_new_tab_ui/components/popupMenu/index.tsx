// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import EllipsisIcon from './ellipsisIcon'
import * as S from './style'
import useCloseWhenUnfocused from './useCloseWhenUnfocused'

interface MenuItemData {
  onClick: Function
  child: JSX.Element
  key: string
}

interface Props {
  menuItems: MenuItemData[]
  isOpen: boolean
  onClose: () => unknown
}

export default function PopupMenu (props: Props) {
  const ref = React.useRef<HTMLUListElement>(null)
  useCloseWhenUnfocused(ref, props.onClose)
  return (
    <>
    {props.isOpen && props.menuItems.length &&
      <S.Menu
        role='menu'
        ref={ref}
      >
        { props.menuItems.map(item =>
        <S.MenuItem
          key={item.key}
          role='menuitem'
          tabIndex={0}
          onClick={item.onClick.bind(null)}
        >
          {item.child}
        </S.MenuItem>
        )}
      </S.Menu>
    }
    </>
  )
}

interface TriggerProps {
  isOpen: boolean
  onTrigger: Function
}

export const EllipsisTrigger: React.FC<TriggerProps> = (props) => {
  return (
    <S.Trigger>
      <S.IconButton
        isActive={props.isOpen}
        onClick={props.onTrigger.bind(null)}
        aria-haspopup='true'
        aria-expanded={props.isOpen ? 'true' : 'false'}
      >
        <EllipsisIcon />
        {props.children}
      </S.IconButton>
    </S.Trigger>
  )
}
