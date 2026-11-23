// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/nala/tokens/css/variables'

// Shared Styles
import { LinkText } from '$wallet/components/shared/style'

export const CheckboxText = styled.div`
  display: flex;
  flex-direction: column;
  width: 100%;
  height: 100%;
  padding: 0;

  & > p {
    margin: 4px 0 0 0;
    padding-top: 0px;
    padding-left: 12px;
    text-align: left;
    vertical-align: top;
    font: ${leo.font.default.regular};
    color: ${leo.color.text.primary};
  }
`

export const TermsLink = styled(LinkText)`
  color: ${leo.color.text.primary};
  text-decoration: underline;
  font-weight: 400;
`
