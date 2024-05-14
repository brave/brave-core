// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import ProgressRing from '@brave/leo/react/progressRing'
import Icon from '@brave/leo/react/icon'

// Assets
import Lines from '../../../../assets/svg-icons/tx_details_lines.svg'

// Types
import { BraveWallet } from '../../../../constants/types'

// Constants
import { layoutPanelWidth } from '../../wallet-page-wrapper/wallet-page-wrapper.style'

// Shared Styles
import {
  AssetIconFactory,
  AssetIconProps,
  Column,
  Row,
  Text
} from '../../../shared/style'

export const NftIconStyles = { height: 140, width: 140 }

export const ContentWrapper = styled(Column)`
  padding: 0px 32px 32px 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0px 16px 16px 16px;
    overflow-y: auto;
    display: block;
  }
`

export const HeroContainer = styled(Row)`
  background-color: ${leo.color.container.highlight};
  border-radius: 8px;
  height: 140px;
  overflow: hidden;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    height: unset;
    padding: 24px 0px 14px 0px;
  }
`

export const HeroContent = styled(Row)`
  position: absolute;
  padding: 0px 24px 0px 0px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    flex-direction: column;
    position: relative;
    padding: 0px;
  }
`

export const IconAndValue = styled(Row)`
  justify-content: flex-start;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    flex-direction: column;
    align-items: center;
    justify-content: center;
    margin-bottom: 16px;
  }
`

export const NFTIconWrapper = styled(Row)`
  margin: 0px 24px 0px 0px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    margin: 0px;
  }
`

export const TransactionValues = styled(Column)`
  align-items: flex-start;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    align-items: center;
    padding: 0px;
  }
`

export const SectionRow = styled(Row)`
  justify-content: flex-start;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    flex-direction: column;
    align-items: flex-start;
  }
`

export const SectionLabel = styled(Text)`
  color: ${leo.color.text.secondary};
  min-width: 150px;
  line-height: 24px;
`

export const SectionInfoText = styled(Text)`
  color: ${leo.color.text.primary};
  line-height: 24px;
  word-break: break-all;
`

export const HeroBackground = styled.div`
  width: 824px;
  height: 989px;
  background-color: ${leo.color.container.background};
  -webkit-mask-image: url(${Lines});
  mask-image: url(${Lines});
  position: absolute;
  mask-position: center;
  mask-repeat: no-repeat;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    height: 100%;
    mask-size: cover;
  }
`
export const TransactionTypeText = styled(Text)`
  color: ${leo.color.text.primary};
  line-height: 28px;
  text-transform: capitalize;
`

export const TransactionTotalText = styled(Text)`
  color: ${leo.color.text.primary};
  line-height: 24px;
`

export const TransactionFiatText = styled(Text)`
  color: ${leo.color.text.secondary};
  line-height: 18px;
`

const assetIconProps = {
  width: '40px',
  height: 'auto'
}
export const AssetIcon = AssetIconFactory<AssetIconProps>(assetIconProps)

const swapIconProps = {
  width: '24px',
  height: 'auto'
}
export const SwapIcon = AssetIconFactory<AssetIconProps>(swapIconProps)

export const StatusBoxWrapper = styled(Column)`
  align-items: flex-end;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    align-items: center;
  }
`

export const StatusBox = styled.div<{
  status: BraveWallet.TransactionStatus
}>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  padding: 3px 4px;
  border-radius: 4px;
  margin-bottom: 8px;
  background-color: ${(p) =>
    p.status === BraveWallet.TransactionStatus.Confirmed ||
    p.status === BraveWallet.TransactionStatus.Approved
      ? leo.color.systemfeedback.successBackground
      : p.status === BraveWallet.TransactionStatus.Error ||
        p.status === BraveWallet.TransactionStatus.Dropped
      ? leo.color.systemfeedback.errorBackground
      : p.status === BraveWallet.TransactionStatus.Unapproved
      ? leo.color.divider.strong
      : leo.color.systemfeedback.infoBackground};
`

export const StatusText = styled(Text)<{
  status: BraveWallet.TransactionStatus
}>`
  color: ${(p) =>
    p.status === BraveWallet.TransactionStatus.Confirmed ||
    p.status === BraveWallet.TransactionStatus.Approved
      ? leo.color.systemfeedback.successText
      : p.status === BraveWallet.TransactionStatus.Error ||
        p.status === BraveWallet.TransactionStatus.Dropped
      ? leo.color.systemfeedback.errorText
      : p.status === BraveWallet.TransactionStatus.Unapproved
      ? leo.color.text.secondary
      : leo.color.systemfeedback.infoText};
  line-height: normal;
  letter-spacing: 0.4px;
  text-transform: uppercase;
  font-size: 10px;
`

export const LoadingIcon = styled(ProgressRing)<{
  status: BraveWallet.TransactionStatus
}>`
  --leo-progressring-size: 14px;
  --leo-progressring-color: ${(p) =>
    p.status === BraveWallet.TransactionStatus.Unapproved
      ? leo.color.text.secondary
      : leo.color.systemfeedback.infoText};
  margin-right: 4px;
`

export const SuccessIcon = styled(Icon).attrs({
  name: 'check-circle-outline'
})`
  --leo-icon-size: 14px;
  color: ${leo.color.systemfeedback.successIcon};
  margin-right: 4px;
`

export const ErrorIcon = styled(Icon).attrs({
  name: 'warning-circle-outline'
})`
  --leo-icon-size: 14px;
  color: ${leo.color.systemfeedback.errorIcon};
  margin-right: 4px;
`

export const DateText = styled(Text)`
  color: ${leo.color.text.primary};
  line-height: 18px;
  margin-bottom: 4px;
`

export const NetworkNameText = styled(DateText)`
  color: ${leo.color.text.secondary};
  margin-bottom: 0px;
`

export const SpeedupIcon = styled(Icon).attrs({
  name: 'network-speed-fast'
})`
  --leo-icon-size: 20px;
  color: ${leo.color.white};
  margin-right: 8px;
`

export const RetryIcon = styled(Icon).attrs({
  name: 'refresh'
})`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.interactive};
  margin-right: 8px;
`

export const IntentAddressText = styled(Text)`
  color: ${leo.color.text.tertiary};
  line-height: 16px;
`

export const SwapAmountText = styled(Text)`
  color: ${leo.color.text.primary};
  line-height: 24px;
  margin-right: 4px;
`

export const SwapFiatValueText = styled(Text)`
  color: ${leo.color.text.secondary};
  line-height: 24px;
`

export const ArrowIcon = styled(Icon).attrs({
  name: 'arrow-right'
})`
  --leo-icon-size: 14px;
  color: ${leo.color.icon.default};
`

export const RowWrapped = styled(Row)`
  flex-wrap: wrap;
`
