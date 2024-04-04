// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import AlertReact from '@brave/leo/react/alert'

import InfoLogo from '../../../../assets/svg-icons/info-icon.svg'

export const Alert = styled(AlertReact)`
  margin-bottom: 15px;

  &:first-of-type {
    margin: 15px 0px;
  }
`

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  padding: 0px 15px 15px 15px;
  min-height: 320px;
  height: 100%;
`

export const CreateAccountStyledWrapper = styled(StyledWrapper)`
  gap: 14px;
  & > * {
    width: 250px;
  }
`

export const SelectWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  margin-bottom: 15px;
`

export const DisclaimerWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  background-color: ${(p) => p.theme.color.warningBackground};
  border-radius: 8px;
  padding: 10px;
  margin-bottom: 10px;
`

export const DisclaimerText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  text-align: center;
  color: ${(p) => p.theme.color.text02};
  text-transform: none;
  font-weight: 400;
  width: 100%;
`

export const InfoIcon = styled.div`
  width: 12px;
  height: 12px;
  margin-top: 4px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${InfoLogo});
  mask-image: url(${InfoLogo});
`

export const ImportRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  margin-bottom: 15px;
  width: 250px;
`

export const ImportButton = styled.label`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  border-radius: 4px;
  padding: 4px 6px;
  outline: none;
  background-color: transparent;
  border: ${(p) => `1px solid ${p.theme.color.interactive08}`};
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  margin-right: 10px;
  color: ${(p) => p.theme.color.interactive07};
`

export const ErrorText = styled.span`
  font: ${leo.font.default.regular};
  color: ${leo.color.systemfeedback.errorText};
`

export const SubmitButtonWrapper = styled.div`
  display: flex;
`
