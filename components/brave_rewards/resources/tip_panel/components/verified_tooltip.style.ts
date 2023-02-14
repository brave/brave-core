/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const title = styled.div`
  display: flex;
  align-items: center;
  gap: 9px;
  font-weight: 400;
  font-size: 14px;
  line-height: 24px;
  color: var(--color-text-primary);
`

export const checkmark = styled.div`
  width: 15px;
  height: 18px;
  color: var(--color-icon-interactive);
`

export const text = styled.div`
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  color: var(--color-text-secondary);
`
