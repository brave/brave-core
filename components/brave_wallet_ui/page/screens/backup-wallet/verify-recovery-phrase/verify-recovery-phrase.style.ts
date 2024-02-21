// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Input from '@brave/leo/react/input'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css'

export const ErrorTextRow = styled.div<{
  hasError: boolean
}>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  height: ${(p) => (p.hasError ? 'auto' : '38px')};
  margin-top: 20px;
  padding-top: 10px;
  margin-bottom: 12px;
`

export const FormLabel = styled.span`
  font-family: Poppins;
  font-size: 16px;
  font-style: normal;
  font-weight: 400;
  line-height: 28px;
  color: ${leo.color.text.primary};
  margin-bottom: 30px;
`

export const Bold = styled(FormLabel)`
  font-weight: 600;
`

export const FormInput = styled(Input).attrs({
  mode: 'filled'
})`
  width: 100%;
`

export const ErrorAlert = styled(Alert).attrs({
  kind: 'error',
  mode: 'simple'
})`
  --leo-alert-center-position: 'center';
  --leo-alert-center-width: '100%';
  width: 100%;

  leo-alert {
    align-items: center;
  }
`

export const CloseButton = styled(Button).attrs({
  kind: 'plain'
})`
  --leo-button-padding: 0;
  height: 20px;
  padding-top: 2px;
`

export const CloseIcon = styled(Icon).attrs({
  name: 'close'
})`
  --leo-icon-color: ${leo.color.systemfeedback.errorIcon};
`

export const BackButton = styled(Button).attrs({
  kind: 'plain'
})`
  width: '100%';
`
