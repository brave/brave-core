/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface PropsStatus {
  isVerified: boolean
}

export const StyledWrapper = styled('div')<{}>`
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100vh;
  background: rgba(0, 0, 0, 0);
  z-index: 99;
  padding: 0 20px;
  overflow: hidden;
`

export const StyledDialog = styled('div')<{}>`
  margin: 52px auto;
  background: ${p => p.theme.palette.white};
  border-radius: 6px;
  overflow: hidden;
  position: relative;
  box-shadow: 0 0 12px 0 rgba(12, 13, 33, 0.44);
`

export const StyledContent = styled('div')<{}>`
  padding: 20px;
`

export const StyledHeader = styled('div')<{}>`
  font-weight: bold;
  border-bottom: 1px solid ${p => p.theme.palette.grey400};
  padding-bottom: 5px;
  margin-bottom: 10px;
`

export const StyledStatus = styled('div')<PropsStatus>`
  font-weight: normal;
  float: right;
  color: ${p => p.isVerified ? p.theme.palette.green600 : p.theme.palette.black};
`

export const StyledIcon = styled('span')<{}>`
  vertical-align: middle;
  display: inline-block;
  width: 24px;
  height: 24px;
  margin-left: -5px;

  .icon {
    height: 20px;
    width: auto;
  }
`
