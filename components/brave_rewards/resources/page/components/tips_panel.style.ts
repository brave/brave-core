/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../shared/lib/css_mixins'

export const root = styled.div``

export const description = styled.div`
  color: var(--brave-palette-neutral600);
  margin: 16px 0;
  line-height: 24px;
`

export const showAll = styled.div`
  margin-top: 25px;
  text-align: right;

  button {
    ${mixins.buttonReset}
    cursor: pointer;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    letter-spacing: -0.01em;
    color: #4C54D2;
  }
`
