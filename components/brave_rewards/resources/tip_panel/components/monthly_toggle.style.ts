/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/variables'

export const root = styled.div`
  display: flex;
  gap: 8px;
  align-items: center;

  --toggle-handle-margin: 3px;
  --toggle-button-height: 24px;
  --toggle-button-width: 40px;
  --toggle-button-handle-margin: 2px;
  --toggle-button-color: ${leo.color.button.background};
`

export const label = styled.div`
  flex: 1 1 auto;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.primary};
`

export const info = styled.div`
  position: relative;
  height: 17px;
  width: 17px;
  color: ${leo.color.icon.default};

  .tooltip {
    --tooltip-offset: -45px;
    --tooltip-width: 280px;

    visibility: hidden;
    transition: visibility 0s linear 300ms;
    font-size: 14px;
    line-height: 24px;
    color: ${leo.color.text.primary};
  }

  &:hover {
    .tooltip {
      visibility: visible;
    }
  }
`
