// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Dialog from '@brave/leo/react/dialog'
import { font, spacing, color } from '@brave/leo/tokens/css/variables'

export const TermsDialog = styled(Dialog)`
  --leo-dialog-padding: ${spacing['2Xl']};
`

export const Title = styled.h2`
  font: ${font.heading.h3};
  margin: 0;
  padding: 0;
`

export const TermsText = styled.p`
  font: ${font.default.regular};
  padding: ${spacing['3Xl']} 0;
  margin: 0;
`

export const TermsLabel = styled.span`
  font: ${font.default.regular};
  margin: 0;
  padding: 0;

  & a {
    color: ${color.text.interactive};
    text-decoration: none;
    font: ${font.default.semibold};
  }
`
