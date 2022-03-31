/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled('div')<{}>`
  font-family: ${p => p.theme.fontFamily.heading};
`

export const StyledTitle = styled('div')<{}>`
  font-size: 20px;
  font-weight: 600;
  line-height: 2;
  color: ${p => p.theme.palette.grey800};
  margin-bottom: 20px;
  text-align: center;
`

export const StyledLoader = styled('div')<{}>`
  margin: 0 auto;
  width: 30px;
  height: 30px;
`

export const StyledError = styled('div')<{}>`
  text-align: center;

  a {
    color: ${p => p.theme.palette.blurple500};
    text-decoration: none;

    :hover {
      text-decoration: underline;
    }
  }
`

export const StyledButton = styled('div')<{}>`
  display: flex;
  margin-top: 20px;
  flex-direction: column;
  align-items: center;
`

export const StyledLink = styled('a')<{}>`
  text-decoration: none;
  cursor: pointer;
  color: ${p => p.theme.palette.blurple500};
  font-weight: 600;
`
