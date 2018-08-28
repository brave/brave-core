/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface StyleProps {
  show?: boolean
  size?: string
}

export const StyledWrapper = styled<{}, 'div'>('div')`
  display: block;
  width: 100%;
  position: relative;
` as any

export const StyledTransitionWrapper = styled<StyleProps, 'div'>('div')`
  height: ${p => p.show ? '100%' : '0'};
  opacity: ${p => p.show ? '1' : '0'};
  overflow: ${p => p.show ? 'unset' : 'hidden'};
  transition: visibility 0.5s, opacity 0.5s linear;
` as any

export const StyledToggleWrapper = styled<StyleProps, 'div'>('div')`
  width: 100%;
  display: block;
  padding: ${p => p.show ? '20px 30px' : '20px'};
  top: ${p => p.show ? 'unset' : '12px'};
  position: ${p => p.show ? 'static' : 'absolute'};
  background: ${p => p.show ? '#E9EBFF' : 'inherit'};
` as any

export const StyledSummaryText = styled<{}, 'span'>('span')`
  color: #A1A8F2;
  font-size: 14px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.35px;
  line-height: 22px;
` as any

export const StyledArrowIcon = styled<StyleProps, 'span'>('span')`
  width: 25px;
  height: 25px;
  display: block;
  margin-left: 32px;
  margin-top: ${p => p.show ? '-2px' : '-22px'};
  color: #696FDC;
` as any

export const StyledGrid = styled<{}, 'div'>('div')`
  display: flex;
  flex-direction: row;
  justify-content: flex-end;
`

export const StyledColumn = styled<StyleProps, 'div'>('div')`
  flex: ${p => p.size} 0 0;
`
