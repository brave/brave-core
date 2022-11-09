// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import FlashDriveIcon from '../../../assets/svg-icons/graphic-flashdrive-icon.svg'
import { WalletButton } from '../../shared/style'

interface StyleProps {
  isConnected: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: ${(p) => p.theme.color.background01};
`

export const Title = styled.span`
  font-family: Poppins;
  font-size: 15px;
  font-weight: 600;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.04em;
  margin-bottom: 5px;
  width: 250px;
  text-align: center;
`

export const Description = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.01em;
`

export const PageIcon = styled.div`
  width: 120px;
  height: 120px;
  mask-size: 100%;
  background-color: ${(p) => p.theme.color.text02};
  -webkit-mask-image: url(${FlashDriveIcon});
  mask-image: url(${FlashDriveIcon});
  margin-bottom: 35px;
`

export const InstructionsButton = styled(WalletButton)`
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-family: Poppins;
  font-style: normal;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.interactive05};
  margin-bottom: 35px;
`

export const ButtonWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  padding-left: 8px;
`

export const Indicator = styled.div<StyleProps>`
  width: 15px;
  height: 15px;
  border-radius: 100%;
  background-color: ${(p) => p.isConnected ? p.theme.color.successBorder : p.theme.color.errorBorder};
  margin-right: 8px;
`

export const ConnectionRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  width: 100%;
  margin-bottom: 10px;
`
