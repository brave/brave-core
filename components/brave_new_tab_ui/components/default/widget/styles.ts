/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface WidgetContainerProps {
  showWidget: boolean
  menuPosition: 'right' | 'left'
  textDirection: string
}

export const StyledWidgetContainer = styled<WidgetContainerProps, 'div'>('div')`
  display: ${p => p.showWidget ? 'flex' : 'none'};
  align-items: center;
  flex-direction: ${p => p.menuPosition === 'right' ? 'row' : 'row-reverse'}

  @media screen and (max-width: 1150px) {
    flex-direction: row;
    padding: ${p => p.textDirection === 'ltr'
    ? `0 0 0 48px`
    : `0 48px 0 0`}
  }
`

interface WidgetVisibilityProps {
  widgetMenuPersist: boolean
}

export const StyledWidget = styled<WidgetVisibilityProps, 'div'>('div')`
  padding: 24px;

  ${StyledWidgetContainer}:hover & {
    border-radius: 16px;
    background: rgba(33, 37, 41, 0.48);
  }

  // Also hover when menu button has been clicked
  ${ p => p.widgetMenuPersist && `
    border-radius: 16px;
    background: rgba(33, 37, 41, 0.48);
  `}

`

export const StyledWidgetMenuContainer = styled<WidgetVisibilityProps, 'div'>('div')`
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

`

interface WidgetMenuProps {
  menuPosition: 'right' | 'left'
  textDirection: string
}

export const StyledWidgetMenu = styled<WidgetMenuProps, 'div'>('div')`
  position absolute;
  width: 166px;
  padding: 8px 0;
  background: white;
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
  blackground: white;
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
    background: rgb(217, 221, 254);
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
