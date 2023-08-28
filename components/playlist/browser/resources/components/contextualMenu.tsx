// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

import Icon from '@brave/leo/react/icon'
import { color } from '@brave/leo/tokens/css'

interface MenuItemProps {
  name: string
  iconName: string
  onClick: () => void
}

interface MenuProps {
  items: MenuItemProps[]
  onShowMenu?: () => void
  onDismissMenu?: () => void
}

const StyledRow = styled.button`
  display: contents;
  text-align: start;
  white-space: nowrap;
  cursor: pointer;
  /* TODO(sko) Check if we need hovered background */
`

const StyledMenuIcon = styled(Icon)`
  --leo-icon-size: 16px;
`

function ContextualMenuItem ({
  name,
  iconName,
  onClick,
  onClickOutside
}: MenuItemProps & { onClickOutside: () => void }) {
  return (
    <StyledRow
      onClick={e => {
        e.stopPropagation()
        onClick()
        onClickOutside()
      }}
    >
      <span>{name}</span>
      <StyledMenuIcon name={iconName} />
    </StyledRow>
  )
}

const StyledContextualMenu = styled.div`
  position: absolute;
  right: 0;

  display: grid;
  grid-template-columns: 1fr 16px;
  grid-auto-rows: 28px;
  width: fit-content;
  gap: 0px 8px;
  align-items: center;
  z-index: 5;

  padding: 8px 16px;
  border-radius: 8px;
  border: 1px solid ${color.gray[20]};
  background: ${color.white};
`

function ContextualMenu ({
  items,
  onClickOutside
}: MenuProps & { onClickOutside: () => void }) {
  React.useEffect(() => {
    document.addEventListener('click', onClickOutside)
    // Playlist has an <iframe> that holds video player. When it's clicked,
    // we should dismiss the menu but 'click' event won't be notified. Instead
    // of it, uses 'blur' event while it could have a little side effect.
    window.addEventListener('blur', onClickOutside)
    return () => {
      document.removeEventListener('click', onClickOutside)
      window.removeEventListener('blur', onClickOutside)
    }
  }, [onClickOutside])

  return (
    <StyledContextualMenu>
      {items.map(item => (
        <ContextualMenuItem
          {...item}
          key={item.name}
          onClickOutside={onClickOutside}
        />
      ))}
    </StyledContextualMenu>
  )
}

const StyledAnchorButton = styled.div`
  position: relative;
  cursor: pointer;
`

const StyledLeoButtonContainer = styled.button`
  display: contents;
  color: ${color.text.secondary};
`

export default function ContextualMenuAnchorButton ({
  items,
  onShowMenu,
  onDismissMenu
}: MenuProps) {
  const [showingMenu, setShowingMenu] = React.useState(false)
  return (
    <StyledAnchorButton>
      <StyledLeoButtonContainer
        onClick={e => {
          e.stopPropagation()
          setShowingMenu(true)
          if (onShowMenu) onShowMenu()
        }}
      >
        <Icon name='more-horizontal' />
      </StyledLeoButtonContainer>
      {showingMenu && (
        <ContextualMenu
          items={items}
          onClickOutside={() => {
            setShowingMenu(false)
            if (onDismissMenu) onDismissMenu()
          }}
        />
      )}
    </StyledAnchorButton>
  )
}
