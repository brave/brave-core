/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/variables'

import { buttonReset } from '../../shared/lib/css_mixins'
import selfCustodyImage from '../../shared/assets/self_custody_invitation.svg'

export const root = styled.div`
  background: no-repeat center 50px/auto 75px url(${selfCustodyImage}),
              ${leo.color.container.background};
  padding: 137px 24px 48px;
  position: relative;
  display: flex;
  flex-direction: column;
  gap: 12px;

  color: ${leo.color.container.background};
  text-align: center;
`

export const close = styled.div`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
  position: absolute;
  right: 16px;
  top: 17px;

  button {
    ${buttonReset}
    cursor: pointer;
  }
`

export const header = styled.div`
  color: ${leo.color.text.primary};
  font: ${leo.font.heading.h4};
`

export const text = styled.div`
  color: ${leo.color.text.secondary};
  font: ${leo.font.default.regular};
  padding-bottom: 2px;
`

export const connect = styled.div`
  display: flex;
  gap: 8px;
  align-items: center;
  --leo-icon-size: 20px;
`

export const dismiss = styled.div`
  button {
    ${buttonReset}
    color: ${leo.color.text.secondary};
    cursor: pointer;
    padding: 8px 0;
    font: ${leo.font.components.buttonDefault};
  }
`
