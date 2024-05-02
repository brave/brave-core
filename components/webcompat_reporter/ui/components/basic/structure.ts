/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import Button from '$web-components/button'

export const SideBySideButtons = styled('div')<{}>`
  box-sizing: border-box;
  display: flex;
  flex-direction: row;
  justify-content: flex-end;
  margin-top: 10px;
`

export const ModalLayout = styled('div')<{}>`
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
  align-items: stretch;
  justify-content: space-around;
  min-height: 100px;
`

export const PaddedButton = styled(Button)<{}>`
  margin: 5px;
`

export const TextSection = styled('div')<{}>`
  margin: 6px 0 !important;
`

export const IconTitle = styled('div')<{}>`
  display: flex;
  flex-direction: row;
`

export const FieldCtr = styled.div`
  margin-top: 8px;
`
