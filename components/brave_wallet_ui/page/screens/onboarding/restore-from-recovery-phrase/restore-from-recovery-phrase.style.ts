// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'

export const RecoveryBaseCss = css`
  box-sizing: border-box;
  width: 100%;
  border: none;
  border-radius: 4px;
  text-align: left;
  vertical-align: middle;
  line-height: 40px;
  word-break: break-word;
  padding-left: 16px;
  padding-right: 16px;
  padding-top: 8px;
  padding-bottom: 8px;

  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  letter-spacing: 0.01em;

  color: ${(p) => p.theme.color.text01};
  background-color: ${(p) => p.theme.color.background02};
`

export const RecoveryTextArea = styled.textarea`
  ${RecoveryBaseCss}
  padding: 16px;
  height: 166px;
`

export const RecoveryTextInput = styled.input`
  ${RecoveryBaseCss}
  font-weight: 800;
`

export const CheckboxText = styled.div`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
`
