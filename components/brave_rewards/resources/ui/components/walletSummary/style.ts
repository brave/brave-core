/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface StyleProps {
  compact?: boolean
}

const getGradientRule = (gradient: string) => {
  return `linear-gradient(-180deg, rgba(${gradient},1) 0%, rgba(255,255,255,1) 70%)`
}

export const StyledWrapper = styled<StyleProps, 'div'>('div')`
  padding: ${p => p.compact ? '0px 7px 0px' : '0px'};
  background: ${p => p.compact ? getGradientRule('233, 235, 255') : 'inherit'};
`

export const StyledInner = styled<StyleProps, 'div'>('div')`
  padding: 14px;
  font-family: Poppins, sans-serif;
`
export const StyledSummary = styled<{}, 'div'>('div')`
  font-size: 14px;
  font-weight: 600;
  line-height: 1.57;
  letter-spacing: 0.4px;
  color: #a1a8f2;
  text-transform: uppercase;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 28px;
  font-weight: 300;
  line-height: 0.79;
  letter-spacing: 0.4px;
  color: #4c54d2;
  margin: 4px 0 26px;
  text-transform: uppercase;
`

export const StyledActivity = styled<{}, 'button'>('button')`
  font-size: 12px;
  color: #686978;
  margin-top: 26px;
  text-align: center;
  padding: 0;
  border: none;
  background: none;
  width: 100%;
  cursor: pointer;
`

export const StyledActivityIcon = styled<{}, 'span'>('span')`
  vertical-align: middle;
  margin-right: 11px;
  width: 22px;
  height: 24px;
  color: #A1A8F2;
  display: inline-block;
`
