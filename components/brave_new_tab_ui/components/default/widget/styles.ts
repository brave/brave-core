// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'brave-ui/theme'

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
  position: relative;
`

export const StyledWidgetMenuContainer = styled<{}, 'div'>('div')`
  position: absolute;
  top: 5px;
  right: 5px;
`

interface WidgetVisibilityProps {
  widgetMenuPersist: boolean
  preventFocus?: boolean
  isCrypto?: boolean
  isCryptoTab?: boolean
  isForeground?: boolean
}

export const StyledWidget = styled<WidgetVisibilityProps, 'div'>('div')`
  padding: ${p => p.isCrypto ? 0 : 24}px;
  max-width: 100%;
  min-width: ${p => p.isCrypto ? '284px' : 'initial'};
  position: relative;
  transition: background 0.5s ease;
  border-radius: ${p => p.isCrypto ? '6px' : '16px'};

  ${StyledWidgetMenuContainer}:hover & {
    background: rgba(33, 37, 41, 0.48);
  }

  // Also hover when menu button has been clicked
  ${ p => (p.widgetMenuPersist && !p.isCryptoTab) && `
    background: rgba(33, 37, 41, 0.48);
  `}
`

interface WidgetMenuProps {
  menuPosition: 'right' | 'left'
  textDirection: string
}

export const StyledWidgetMenu = styled<WidgetVisibilityProps & WidgetMenuProps, 'div'>('div')`
  position absolute;
  width: max-content;
  min-width: 166px;
  padding: 8px 0;
  background-color: ${p => p.theme.color.contextMenuBackground};
  color:  ${p => p.theme.color.contextMenuForeground};
  box-shadow: 0px 0px 6px 0px rgba(0, 0, 0, 0.3);
  border-radius: 4px;
  top: 48px;
  z-index: 4;
  visibility: hidden;
  pointer-events: none;

  ${p => p.widgetMenuPersist && `
    visibility: visible;
    pointer-events: auto;
  `}

  @media screen and (min-width: 1150px) {
    ${p => (p.menuPosition === 'right' && p.textDirection === 'ltr') || (p.menuPosition === 'left' && p.textDirection === 'rtl')
    ? 'left: 8px'
    : 'right: 8px'}
  }

  @media screen and (min-width: 950px) and (max-width: 1150px) {
    ${p => p.textDirection === 'ltr'
    ? `left: ${p.menuPosition === 'left' ? -130 : 8}px`
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
  cursor: pointer;

  &:hover {
    background-color: ${p => p.theme.color.contextMenuHoverBackground};
    color: ${p => p.theme.color.contextMenuHoverForeground};
  }
`

export const StyledWidgetLink = styled<WidgetButtonProps, 'a'>('a')`
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
  cursor: pointer;
  text-decoration: none;

  &:hover {
    background-color: ${p => p.theme.color.contextMenuHoverBackground};
    color: ${p => p.theme.color.contextMenuHoverForeground};
  }
`

interface WidgetIconProps {
  isRefresh?: boolean
  isBinance?: boolean
}

export const StyledEllipsis = styled<WidgetVisibilityProps, 'div'>('div')`
  visibility: hidden;
  pointer-events: none;

  ${p => (p.widgetMenuPersist || p.isForeground) && `
    visibility: visible;
    pointer-events: auto;
  `}

  ${StyledWidgetContainer}:hover & {
    visibility: visible;
    pointer-events: auto;
  }
`

export const StyledWidgetIcon = styled<WidgetIconProps, 'div'>('div')`
  height: 13px;
  width: 13px;
  margin: ${p => p.isBinance ? '0px 13px 0 12px' : '-7px 15px 0 10px'};
  margin-left: ${p => p.isRefresh ? '13px' : p.isBinance ? '12px' : '10px'};

  svg {
    fill: ${p => p.theme.color.contextMenuForeground};
  }
`
export const StyledSpan = styled<{}, 'span'>('span')`
  text-align: left;
  margin-right: 10px;
`
