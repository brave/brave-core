/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import Button from '@brave/leo/react/button'
import styled from 'styled-components'
//import Button from '$web-components/button'

export const TextSection = styled('div') <{}>`
 margin: 6px 0 !important;
`
export const Container = styled('div') <{}>`
 margin: 24px !important;
`

export const HorizontalContainer = styled('div') <{}>`
 display: flex;
 flex-direction: row;
`
export const LeftAlignedItem = styled('div') <{}>`
`
export const RightAlignedItem = styled('div') <{}>`
 margin-left: auto;
 text-align: right;
`
export const SideBySideButtons = styled('div')<{}>`
  box-sizing: border-box;
  display: flex;
  flex-direction: row;
  justify-content: flex-end;
  margin-top: 10px;
`

export const PaddedButton = styled(Button)<{}>`
  margin: 5px;
  --leo-button-radius: 12px;
`