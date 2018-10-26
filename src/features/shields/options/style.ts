/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'
import { OptionsProps, OptionProps } from './index'

export const StyledOptions = styled<OptionsProps, 'div'>('div')`
  position: absolute;
  width: 100%;
  bottom: 58px;
  border-radius: 3px;
  box-shadow: 0 2px 5px 0 rgba(223, 223, 232, 0.5);
  background-color: #fff;
  border: solid 1px #dfdfe8;
  overflow: hidden;
  z-index: 2;
  display: ${p => p.visible ? 'block' : 'none'};
  padding: 9px 0;
`

export const StyledOption = styled<OptionProps, 'div'>('div')`
  font-size: 14px;
  line-height: 36px;
  color: #1b1d2f;
  position: relative;
  padding: 0 0 0 4px;
  display: flex;
  background-color: ${p => p.selected ? '#e9f0ff' : '#fff'};

  &:hover {
    background-color: #e9f0ff;
  }
`

export const StyledOptionCheck = styled<{}, 'div'>('div')`
  flex-basis: 11px;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  color: #A1A8F2;
`

export const StyledOptionText = styled<{}, 'div'>('div')`
  flex-grow: 1;
  padding: 0 21px 0 6px;
  overflow: hidden;
  white-space: nowrap;
  text-overflow: ellipsis;
  user-select: none;
`
