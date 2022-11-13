// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { AssetIconProps, AssetIconFactory, WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  background-color: ${(p) => p.theme.color.background01};
`

export const TopWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
`

export const TopRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  padding-top: 15px;
  padding-left: 30px;
  margin-bottom: 40px;
`

export const Title = styled.span`
  font-family: Poppins;
  font-size: 15px;
  font-weight: 600;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.04em;
  margin-bottom: 10px;
  text-align: center;
`

export const Description = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  color: ${(p) => p.theme.color.text02};
  letter-spacing: 0.01em;
  text-align: center;
  margin-bottom: 40px;
`

export const TokenName = styled.span`
  font-family: Poppins;
  font-size: 14px;
  font-weight: 600;
  line-height: 20px;
  color: ${(p) => p.theme.color.text02};
  letter-spacing: 0.01em;
  margin-top: 15px;
  margin-bottom: 4px;
  text-align: center;
`

export const ContractAddress = styled(WalletButton)`
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.interactive05};
  margin: 0px;
  padding: 0px;
`

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '60px',
  height: 'auto'
})

export const ButtonWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  margin-bottom: 14px;
`

export const NetworkText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`
