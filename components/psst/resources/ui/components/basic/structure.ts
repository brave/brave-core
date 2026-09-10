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
  display: flex;
  justify-content: flex-end;
  align-items: center;
  gap: var(--leo-spacing-m);
  > * {
    flex-grow: 0;
  }
`
export const PsstDlgButton = styled(Button)<{}>`
  padding-left: var(--leo-spacing-m);
  min-height: 44px;
`
