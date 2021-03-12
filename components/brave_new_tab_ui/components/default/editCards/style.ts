/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import palette from 'brave-ui/theme/colors'

export const StyledWrapper = styled('button')<{}>`
  width: 100%;
  border: none;
  display: block;
  margin-top: 5px;
  appearance: none;
  border-radius: 8px;
  cursor: pointer;
  backdrop-filter: blur(23px);
  padding: 12px 20px;
  background: linear-gradient(90deg, rgba(33, 37, 41, 0.6) 0%, rgba(33, 37, 41, 0.24) 100%);
  outline: none;

  &:hover {
    background: rgba(33, 37, 41, 0.5);
  }

  &:focus-visible {
    box-shadow: 0 0 0 1px ${p => p.theme.color.brandBrave};
  }
`

export const StyledTitle = styled('div')<{}>`
  display: flex;
  align-items: center;
  width: fit-content;
  font-size: 13px;
  font-weight: 600;
  color: ${palette.white};
  margin: 0 auto;
  font-family: ${p => p.theme.fontFamily.heading};
`

export const StyledEditIcon = styled('img')<{}>`
  width: 14px;
  margin-right: 10px;
`
