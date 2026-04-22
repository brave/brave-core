/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import Button from '@brave/leo/react/button'
import styled from 'styled-components'

export const Container = styled.div`
  margin: 24px;
`
export const RightAlignedItem = styled.div`
  margin-left: auto;
  text-align: right;
`
export const PsstDlgButton = styled(Button)<{}>`
  padding: 12px;
  min-height: 44px;
`
