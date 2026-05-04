// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import LeoInput from '@brave/leo/react/input'
import LeoButton from '@brave/leo/react/button'

// Assets
import InfoLogo from '../../../../assets/svg-icons/info-icon.svg'

// Shared Styles
import { Column, Text } from '../../../shared/style'
import { layoutPanelWidth } from '../../wallet-page-wrapper/wallet-page-wrapper.style'

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

export const ImportButton = styled(LeoButton)`
  flex-grow: 0;
`

export const ErrorText = styled.span`
  font: ${leo.font.default.regular};
  color: ${leo.color.systemfeedback.errorText};
`

export const CreateAccountWrapper = styled(Column)`
  gap: 32px;
  padding: 0px 32px 32px 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    gap: 20px;
    padding: 0px 20px 20px 20px;
  }
`

export const CreateAccountContent = styled(Column)`
  max-width: 440px;
`

export const Input = styled(LeoInput)`
  width: 100%;
  color: ${leo.color.text.primary};
`

export const NetworkIcon = styled.img`
  width: 45px;
  height: 45px;
`

export const NetworkName = styled(Text)`
  font: ${leo.font.default.semibold};
  letter-spacing: ${leo.typography.letterSpacing.default};
`

export const NetworkDescription = styled(Text)`
  font: ${leo.font.default.regular};
  letter-spacing: ${leo.typography.letterSpacing.default};
`

export const FileNameText = styled(Text)`
  font: ${leo.font.default.regular};
  letter-spacing: ${leo.typography.letterSpacing.default};
  text-transform: none;
`

export const JsonFileLabel = styled(Text)`
  font: ${leo.font.small.semibold};
  letter-spacing: ${leo.typography.letterSpacing.small};
`
