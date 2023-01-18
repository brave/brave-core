/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  margin: 0 30px;
  max-width: 350px;
  border-radius: 8px;
  overflow: hidden;

  .vbat-notice-header {
    font-size: 16px;
    line-height: 24px;
  }

  .vbat-notice-actions {
    display: block;
    text-align: center;
  }

  .vbat-notice-connect button {
    width: 100%;
    margin-bottom: 10px;
  }
`
