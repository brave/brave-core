// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled, { css } from 'brave-ui/theme'

interface WidgetPositionProps {
  menuPosition: 'right' | 'left'
}
interface WidgetContainerProps extends WidgetPositionProps {
  textDirection: string
}

export const StyledWidgetContainer = styled<WidgetContainerProps, 'div'>('div')`
  display: inline-flex;
  /* For debug: */
  /* outline: 1px solid rgba(0, 185, 0, .6); */
  align-items: center;
  flex-direction: ${p => p.menuPosition === 'right' ? 'row' : 'row-reverse'};
  height: fit-content;
  min-width: 0;
`

interface WidgetVisibilityProps {
  widgetMenuPersist: boolean
  preventFocus?: boolean
}

export const StyledWidget = styled<WidgetVisibilityProps, 'div'>('div')`
  padding: 24px;
  max-width: 100%;
  position: relative;
  ${p => !p.preventFocus && css`
    ${StyledWidgetContainer}:hover & {
      border-radius: 16px;
      background: rgba(33, 37, 41, 0.48);
    }
  `}

  // Also hover when menu button has been clicked
  ${ p => p.widgetMenuPersist && `
    border-radius: 16px;
    background: rgba(33, 37, 41, 0.48);
  `}

`

export const StyledWidgetMenuContainer = styled<WidgetVisibilityProps & WidgetPositionProps, 'div'>('div')`
  visibility: hidden;
  pointer-events: none;
  position: relative;

  ${StyledWidgetContainer}:hover & {
    visibility: visible;
    pointer-events: auto;
  }

  // Also hover when menu button has been clicked
  ${ p => p.widgetMenuPersist && `
    visibility: visible;
    pointer-events: auto;
  `}

  // Float in gutter
  ${p => p.menuPosition === 'left' ? css`
      margin-left: -48px;
    ` : css`
      margin-right: -48px;
    `}
`

interface WidgetMenuProps {
  menuPosition: 'right' | 'left'
  textDirection: string
}

export const StyledWidgetMenu = styled<WidgetMenuProps, 'div'>('div')`
  position absolute;
  width: 166px;
  padding: 8px 0;
  background-color: ${p => p.theme.color.contextMenuBackground};
  color:  ${p => p.theme.color.contextMenuForeground};
  box-shadow: 0px 0px 6px 0px rgba(0, 0, 0, 0.3);
  border-radius: 4px;
  top: 48px;

  @media screen and (min-width: 1150px) {
    ${p => (p.menuPosition === 'right' && p.textDirection === 'ltr') || (p.menuPosition === 'left' && p.textDirection === 'rtl')
    ? 'left: 8px'
    : 'right: 8px'}
  }

  @media screen and (min-width: 950px) and (max-width: 1150px) {
    ${p => p.textDirection === 'ltr'
    ? 'left: 8px'
    : 'right: 8px'}
  }

  @media screen and (max-width: 950px) {
    ${p => p.textDirection === 'ltr'
    ? 'right: 8px'
    : 'left: 8px'}
  }
`

interface WidgetButtonProps {
  onClick: () => void
}
export const StyledWidgetButton = styled<WidgetButtonProps, 'button'>('button')`
  border-style: none;
  background: transparent;
  padding: 0;
  display: flex;
  height: 30px;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  outline-color: #FF7654;
  outline-width: 2px;
  outline-offset: -3px;

  &:hover {
    background-color: ${p => p.theme.color.contextMenuHoverBackground};
    color: ${p => p.theme.color.contextMenuHoverForeground};
  }
`

export const StyledWidgetIcon = styled<{}, 'div'>('div')`
  height: 13px;
  width: 13px;
  margin: 0 10px 0 18px;
`
export const StyledSpan = styled<{}, 'span'>('span')`
  height: 13px;
`
