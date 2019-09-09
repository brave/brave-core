/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'brave-ui/theme'
import { TextSection } from './structure'
import { CheckCircleIcon } from 'brave-ui/components/icons'

export const ModalTitle = styled(TextSection)`
  box-sizing: border-box;
  font-size: 20px;
  font-family: ${p => p.theme.fontFamily.heading};
  line-height: 20px;
  font-weight: 500;
  color: ${p => p.theme.color.text};
  margin: 0;
`

export const InfoText = styled(TextSection)`
  box-sizing: border-box;
  color: ${p => p.theme.color.text};
  font-size: 14px;
  font-family: ${p => p.theme.fontFamily.body};
  margin: 0;
  line-height: 1.2;
`

export const DisclaimerText = styled(TextSection)`
  box-sizing: border-box;
  color: ${p => p.theme.color.text};
  font-size: 12px;
  font-family: ${p => p.theme.fontFamily.body};
  margin: 0;
  line-height: 1.2;
`

export const NonInteractiveURL = styled<{}, 'p'>('p')`
  box-sizing: border-box;
  color: ${p => p.theme.color.brandBrave};
  font-size: 14px;
  font-weight: 500;
  display: inline-block;
  text-align: left;
  width: fit-content;
`

export const SuccessIcon = styled(CheckCircleIcon)`
  color: ${p => p.theme.color.subtle};
  width: 30px;
  margin-right: 10px;
`
