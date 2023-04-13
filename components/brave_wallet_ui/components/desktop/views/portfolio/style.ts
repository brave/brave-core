// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { ArrowUpIcon } from 'brave-ui/components/icons'
import { AssetIconProps, AssetIconFactory, WalletButton } from '../../../shared/style'
import More from '../../../extension/assets/actions.svg'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  height: 100%;
`

export const TopRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;
`

export const BalanceTitle = styled.span`
  font-family: Poppins;
  font-size: 15px;
  font-weight: normal;
  color: ${(p) => p.theme.color.text03};
`

export const BalanceText = styled.span`
  font-family: Poppins;
  font-size: 32px;
  font-weight: 600;
  margin-bottom: 20px;
  color: ${(p) => p.theme.color.text01};
`

export const PriceText = styled.span`
  font-family: Poppins;
  font-size: 24px;
  font-weight: 600;
  line-height: 36px;
  letter-spacing: 0.02em;
  margin-right: 10px;
  color: ${(p) => p.theme.color.text01};
`

export const ButtonRow = styled.div<{ noMargin?: boolean }>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  margin: ${(p) => p.noMargin ? '0px' : '20px 0px'};
`

export const BalanceRow = styled.div<{ gap?: string }>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  height: 36px;
  vertical-align: middle;
  gap: ${p => p.gap || 0};
`

export const InfoColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  margin: 10px 0px 20px 10px;
`

export const AssetRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  margin-bottom: 20px;
`

export const AssetColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
`

export const PriceRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
`

export const AssetNameText = styled.span`
  font-family: Poppins;
  font-size: 20px;
  line-height: 30px;
  font-weight: 600;
  letter-spacing: 0.02em;
  color: ${(p) => p.theme.color.text01};
`

export const NetworkDescription = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 16px;
  letter-spacing: 0.02em;
  color: ${(p) => p.theme.color.text02};
`

export const DetailText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 400;
  color: ${(p) => p.theme.color.text03};
`

// Construct styled-component using JS object instead of string, for editor
// support with custom AssetIconFactory.
//
// Ref: https://styled-components.com/docs/advanced#style-objects
export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto'
})

export const SubDivider = styled.div`
  width: 100%;
  height: 2px;
  background-color: ${(p) => p.theme.color.divider01};
  margin-bottom: 12px;
`

export const DividerText = styled.span`
  font-family: Poppins;
  font-size: 15px;
  line-height: 20px;
  letter-spacing: 0.04em;
  font-weight: 600;
  margin-bottom: 10px;
  color: ${(p) => p.theme.color.text03};
`

export const PercentBubble = styled.div<{ isDown?: boolean }>`
  display: flex;
  align-items: center;
  justify-conent: center;
  flex-direction: row;
  padding: 4px 8px;
  border-radius: 8px;
  background-color: ${(p) => p.isDown ? '#EE6374' : '#2AC194'};
`

export const PercentText = styled.span`
  font-family: Poppins;
  font-size: 11px;
  line-height: 17px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
`

export const ArrowIcon = styled(ArrowUpIcon) <{ isDown?: boolean }>`
  width: 12px;
  height: 12px;
  margin-right: 2px;
  transform: ${(p) => p.isDown ? 'rotate(270deg)' : 'rotate(90deg)'};
  color: ${(p) => p.theme.palette.white};
`

export const EmptyTransactionContainer = styled.div`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  height: 100px;
`

export const TransactionPlaceholderText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text03};
  margin-left: 10px;
`

export const AssetBalanceDisplay = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text02};
`

export const DividerRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
`

export const Spacer = styled.div`
  display: flex;
  height: 2px;
  width: 100%;
  margin-top: 10px;
`

export const CoinGeckoText = styled.span`
  font-family: Arial;
  font-size: 10px;
  font-weight: normal;
  color: ${(p) => p.theme.color.text03};
  margin: 15px 0px;
`

export const FilterTokenRow = styled.div`
  display: flex;
  flex-direction: row;
  flex-wrap: wrap;
  width: 100%;
  gap: 14px;
`

export const NftMultimedia = styled.iframe<{ visible?: boolean }>`
  width: 100%;
  min-height: ${p => p.visible ? '500px' : '0px'};
  border: none;
  visibility: ${p => p.visible ? 'visible' : 'hidden'};
  margin-bottom: 30px;
  margin-top: 16px;
`

export const BridgeToAuroraButton = styled(WalletButton)`
  display: flex;
  flex-direction: row;
  justify-content: center;
  align-items: center;
  padding: 8px 14px;
  height: 40px;
  cursor: pointer;
  outline: none;
  border-radius: 40px;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 14px;
  line-height: 21px;
  background-color: ${(p) => p.theme.palette.blurple500};
  color: ${(p) => p.theme.palette.white};
  border: none;
  margin-bottom: 32px;
  margin-right: 10px;
`

export const MoreButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  width: 18px;
  height: 18px;
  margin-left: 16px;
  outline: none;
  border: none;
  padding: 0;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${More});
  mask-image: url(${More});
  mask-size: cover;
`
