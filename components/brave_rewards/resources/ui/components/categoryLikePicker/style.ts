/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledCategoryActionOptinLogo = styled('div')<{}>`
  display: inline-block;
`

export const StyledCategoryActionOptoutLogo = styled('div')<{}>`
  display: inline-block;
`

export const StyledCategoryActionOptinButton = styled('div')<{}>`
margin: auto;
  border: none;
  height: 32px;
  width: 32px;
  outline: none;
  cursor: pointer;
  object-fit: cover;
`

export const StyledCategoryActionOptoutButton = styled('div')<{}>`
  border: none;
  height: 31px;
  width: 31px;
  outline: none;
  cursor: pointer;
`

export const StyledCategoryActionOptinFilledButton = styled(StyledCategoryActionOptinButton)`
  color: ${p => p.theme.palette.red500};
`

export const StyledCategoryActionOptoutFilledButton = styled(StyledCategoryActionOptoutButton)`
  color: ${p => p.theme.palette.red500};
`
