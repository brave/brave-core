/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface StyleProps {
  isPanel?: boolean
  fullScreen?: boolean
}

const getBackground = (props: StyleProps) => {
  if (props.fullScreen) {
    return '#fff'
  }

  if (props.isPanel) {
    return 'linear-gradient(-180deg, rgba(255, 255, 255, 1) 50%, rgba(228, 242, 255, 1) 100%)'
  }

  return 'rgba(255, 255, 255, 0.95)'
}

export const StyledWrapper = styled<StyleProps, 'div'>('div')`
  position: ${p => p.fullScreen ? 'fixed' : 'absolute'};
  top: 0;
  left: 0;
  z-index: 6;
  display: flex;
  align-items: stretch;
  align-content: flex-start;
  flex-wrap: wrap;
  overflow: hidden;
  width: 100%;
  padding: 0 52px 20px;
  border-radius: 6px;
  height: ${p => (p.fullScreen || p.isPanel) ? '100%' : 'auto'};
  overflow-y: ${p => p.fullScreen ? 'scroll' : 'hidden'};
  background: ${p => getBackground(p)};
`

export const StyledHeader = styled<{}, 'div'>('div')`
  text-align: center;
  width: 100%;
  margin: 59px 0;
`

export const StyledTitle = styled<StyleProps, 'div'>('div')`
  width: 100%;
  font-size: ${p => p.isPanel ? 20 : 28}px;
  font-weight: ${p => p.isPanel ? 'normal' : 500};
  line-height: 1.29;
  letter-spacing: -0.2px;
  text-align: center;
  color: #fb542b;
`

export const StyledClose = styled<{}, 'button'>('button')`
  top: 16px;
  right: 16px;
  position: absolute;
  padding: 0;
  border: none;
  background: none;
  cursor: pointer;
  color: #9E9FAB;
  width: 20px;
  height: 20px;
`

export const StyledText = styled<{}, 'div'>('div')`
  width: 100%;
  font-family: Muli, sans-serif;
  font-size: 16px;
  font-weight: 300;
  line-height: 1.63;
  text-align: center;
  color: #4b4c5c;
`

export const StyledGrantIcon = styled<{}, 'img'>('img')`
  height: 53px;
  width: 53px;
  margin: 25px auto 15px;
`

export const StyledPanelText = styled<{}, 'div'>('div')`
  padding: 7px;
  font-size: 12px;
  margin: 7px auto 0px;
  background: rgba(241, 241, 245, 0.70);
  border-radius: 8px 8px 8px 8px;
`

export const StyledHint = styled<{}, 'span'>('span')`
  font-weight: 600;
`
