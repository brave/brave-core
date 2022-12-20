/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  background: #fff;
  padding: 32px;
  border-radius: 8px;
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.2);
  min-width: 550px;

  .layout-narrow & {
    min-width: unset;
    margin: 0 8px;
  }
`

export const header = styled.div`
  display: flex;
  gap: 16px;
  align-items: center;
  justify-content: space-between;
`

export const title = styled.div`
  font-weight: 600;
  font-size: 20px;
  line-height: 24px;
  color: #4b4c5c;
`

export const close = styled.div`
  position: relative;
  top: -6px;
  left: 6px;
`
