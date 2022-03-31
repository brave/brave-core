/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { TextSection } from './structure'
import { CheckCircleIcon } from 'brave-ui/components/icons'
import { Card } from 'brave-ui/components'
import { CardProps } from 'brave-ui/components/layout/card'

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

export const NonInteractiveURL = styled('p')<{}>`
  box-sizing: border-box;
  color: ${p => p.theme.color.brandBrave};
  font-size: 14px;
  font-weight: 500;
  display: inline-block;
  text-align: left;
  width: fit-content;
  word-break: break-all;
  text-decoration: underline;
`

export const SuccessIcon = styled(CheckCircleIcon)`
  color: ${p => p.theme.color.subtle};
  width: 30px;
  margin-right: 10px;
`

export const RectangularCard = styled(Card)<CardProps>`
  border-radius: 0;
`

export const Input = styled.input`
  box-sizing: border-box;
  width: 100%;
  outline: none;
  background-color: ${(p) => p.theme.color.background02};
  box-shadow: none;
  border: 1px solid ${(p) => p.theme.color.interactive08};
  border-radius: 4px;
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 13px;
  padding: 10px;
  margin: 0px;
  color: ${(p) => p.theme.color.text01};
  ::placeholder {
    font-family: ${p => p.theme.fontFamily.body};
    font-size: 12px;
    color: ${(p) => p.theme.color.text03};
    font-weight: normal;
  }
  :focus {
      outline: none;
  }
`

export const InputLabel = styled.label`
  display: block;
  box-sizing: border-box;
  color: ${p => p.theme.color.text};
  font-size: 12px;
  font-family: ${p => p.theme.fontFamily.body};
  padding-bottom: 4px;
`

export const TextArea = styled.textarea`
  box-sizing: border-box;
  width: 100%;
  outline: none;
  background-color: ${(p) => p.theme.color.background02};
  box-shadow: none;
  border: 1px solid ${(p) => p.theme.color.interactive08};
  border-radius: 4px;
  resize: none;
  font-size: 13px;
  font-family: ${p => p.theme.fontFamily.body};
  font-weight: normal;
  padding: 10px;
  margin: 0px;
  color: ${(p) => p.theme.color.text01};
  ::placeholder {
      color: ${(p) => p.theme.color.text03};
      font-weight: normal;
      font-size: 12px;
      font-family: ${p => p.theme.fontFamily.body};
  }
  :focus {
      outline: none;
  }
`
