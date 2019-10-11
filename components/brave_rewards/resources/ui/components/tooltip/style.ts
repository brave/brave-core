/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import styled from 'styled-components'

interface StyleProps {
  displayed?: boolean
  rightEdge?: boolean
}

export const StyledWrapper = styled<{}, 'div'>('div')`
  display: inline-block;
  position: relative;
`

export const StyledTooltip = styled<StyleProps, 'div'>('div')`
  left: 50%;
  top: calc(100% + 10px);
  transform: ${p => p.rightEdge ? 'translateX(-80%);' : 'translateX(-50%);'};
  white-space: nowrap;
  position: absolute;
  background: #0C0D21;
  text-align: center;
  padding: 10px;
  z-index: 2;
  border-radius: 3px;
  box-shadow: 1px 1px 5px 0 rgba(34, 35, 38, 0.43);
  display: ${p => p.displayed ? 'inline-block' : 'none'};
`
export const StyledTooltipText = styled<{}, 'div'>('div')`
  color: #FFFFFF;
  font-family: Muli, sans-serif;
  font-weight: 300;
  font-size: 14px;
`

export const StyledPointer = styled<StyleProps, 'div'>('div')`
  width: 0;
  height: 0;
  border-style: solid;
  position: absolute;
  top: -7px;
  left: ${p => p.rightEdge ? 'calc(80% - 10px)' : 'calc(50% - 7px)'};
  border-width: 0 7px 8px 7px;
  border-color: transparent transparent #0C0D21 transparent;
`

export const StyledChildWrapper = styled<{}, 'div'>('div')`
  cursor: pointer;
  height: 24px;
`
