// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'

import ArrowDown2Icon from '../../../assets/svg-icons/arrow-down-2.svg'

// Shared Styles
import {
  AssetIconFactory,
  AssetIconProps,
  Text,
  WalletButton,
} from '../../shared/style'

export const ExchangeRate = styled(Text)`
  display: flex;
  align-items: center;
`

export const SwapDetails = styled.div`
  position: relative;
  border: 1px solid ${leo.color.divider.subtle};
  box-shadow: 0 0 90px rgba(99, 105, 110, 0.08);
  border-radius: 8px;
  height: 220px;
  width: calc(100% - 8px);
  margin: 8px 0;
`

export const SwapDetailsDivider = styled.div`
  position: absolute;
  top: 50%;
  border: 0.5px solid ${leo.color.divider.subtle};
  width: 100%;
`

export const SwapDetailsArrowContainer = styled.div`
  top: calc(50% - 16px); // 16px = half of 32px (height)
  left: calc(50% - 16px); // 16px = half of 32px (width)
  position: absolute;
  border: 1px solid ${leo.color.divider.subtle};
  background-color: ${leo.color.page.background};
  border-radius: 50%;
  width: 32px;
  height: 32px;
  display: flex;
  flex-direction: column;
  justify-content: space-around;
  align-items: center;
`

export const SwapDetailsArrow = styled.div`
  -webkit-mask-image: url(${ArrowDown2Icon});
  mask-image: url(${ArrowDown2Icon});
  background-color: ${leo.color.neutral[30]};
  width: 12px;
  height: 16px;
`

export const SwapAssetContainer = styled.div<{ top: boolean }>`
  width: 100%;
  position: ${(p) => (p.top ? undefined : 'absolute')};
  top: ${(p) => (p.top ? undefined : '50%')};
`

export const SwapAssetHeader = styled.div`
  display: flex;
  justify-content: space-between;
  margin: 8px 12px;
`

export const SwapAssetAddress = styled.div`
  border: 1px solid ${leo.color.divider.subtle};
  border-radius: 4px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  width: fit-content;
`

export const AddressOrb = styled.div<{ orb: string }>`
  width: 12px;
  height: 12px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin: 3px;
`

export const AccountNameText = styled(Text)`
  cursor: default;
  display: flex;
  align-items: center;
  margin-right: 3px;
`

export const SwapAssetDetailsContainer = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  margin: 0 12px;
`

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto',
})

export const SwapAmountColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
`

export const Spacer = styled.div`
  display: flex;
  height: 4px;
`

export const SwapAssetAmountSymbol = styled(Text)`
  display: flex;
  align-items: center;
`

export const NetworkDescriptionText = styled(Text)`
  display: flex;
  align-items: center;
`

export const LaunchButton = styled(WalletButton)`
  font: ${leo.font.small.semibold};
  letter-spacing: 0.01em;
  color: ${leo.color.button.background};
  background: none;
  cursor: pointer;
  outline: none;
  border: none;
  margin: 4px;
  padding: 0px;
`

export const SwapAmountRow = styled.div`
  display: flex;
  align-items: flex-end;
  justify-content: center;
  flex-direction: row;
`
