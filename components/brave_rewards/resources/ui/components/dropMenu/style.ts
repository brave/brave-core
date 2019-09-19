/* This Source Code Form is subject to the terms of the Mozilla Public
* License. v. 2.0. If a copy of the MPL was not distributed with this file.
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledAdMenuDropContent = styled<{}, 'div'>('div')`
  display: block;
  position: absolute;
  box-shadow: 0px 8px 16px 0px rgba(0,0,0,0.2);
  z-index: 1;
  margin-top: -10px;
  background-color: ${p => p.theme.color.contextMenuBackground};
  border: 1px solid;
  border-color: ${p => p.theme.color.inputBorder};
  border-radius: 5px;
  min-width: 200px;
`

export const StyledAdStatBulletMenuIcon = styled<{}, 'div'>('div')`
  width: 32px;
  height: 32px;
  display: inline-block;
  padding: 2px;
  position: relative;
  border: none;
  outline: none;
`

export const StyledAdMenuOptionDropContent = styled<{}, 'div'>('div')`
  display: flex;
  align-items: center;
  height: 35px;
  padding-left: 5px;
  margin-top: auto;
  cursor: pointer;
  &:hover {
    background-color: ${p => p.theme.color.brandBraveLight};
  }
`

export const StyledAdMenuOptionDropContentText = styled<{}, 'span'>('span')`
  font-size: 16px;
  vertical-align: middle;
  font-weight: 600;
`
