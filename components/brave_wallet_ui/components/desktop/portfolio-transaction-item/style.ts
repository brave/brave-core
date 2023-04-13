// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { MoreVertRIcon, ArrowRightIcon } from 'brave-ui/components/icons'
import CoinsIconSVG from '../../../assets/svg-icons/coins-icon.svg'
import { WalletButton } from '../../shared/style'
import { OrbContainer } from '../../extension/transaction-detail-panel/style'

interface StyleProps {
  orb: string
}

export const PortfolioTransactionItemWrapper = styled.div<{ isFocused?: boolean }>`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  margin: 14px 0px;
  position: relative;
  background-color: ${(p) => p.isFocused ? `${p.theme.color.text01}10` : 'none'};
  padding: 10px;
  border-radius: 10px;
  gap: 16px;
`

export const DetailRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  flex-wrap: wrap;
`

export const BalanceAndMoreRow = styled(DetailRow)`
  width: 70%;
`

export const AddressText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
  margin: 0px 5px;
`

export const DetailText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 400;
  color: ${(p) => p.theme.color.text02};
`

export const FromCircle = styled.div<Partial<StyleProps>>`
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-right: 30px;
  @media screen and (max-width: 600px) {
    display: none
  }
`

export const ToCircle = styled.div<Partial<StyleProps>>`
  width: 24px;
  height: 24px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  position: absolute;
  left: 26px;
  @media screen and (max-width: 600px) {
    display: none
  }
`

export const MoreButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
`

export const RejectedTransactionSpacer = styled.div`
  width: 38px;
`

export const MoreIcon = styled(MoreVertRIcon)`
  width: auto;
  height: 26px;
  transform: rotate(90deg);
  color: ${(p) => p.theme.color.interactive08};
`

export const DetailColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
`

export const DetailTextLight = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
  margin-right: 6px;
`

export const DetailTextDark = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  margin-right: 6px;
`

export const DetailTextDarkBold = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text02};
`

export const BalanceColumn = styled.div`
  display: flex;
  align-items: flex-end;
  justify-content: center;
  flex-direction: column;

  & > * {
    overflow-wrap: break-word;
    word-break: break-all;
  }

  /**
    Using a fixed width,
    so that columns remain aligned across tx-item rows
  */
  width: 70%;
`

export const ArrowIcon = styled(ArrowRightIcon)`
  width: auto;
  height: 16px;
  margin-right: 6px;
  color: ${(p) => p.theme.color.text03};
`

export const OrbAndTxDescriptionContainer = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  flex-wrap: wrap;
  width: 50%;
`

export const StatusRow = styled.div`
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  margin-right: 16px;
`

export const CoinsButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: default;
  outline: none;
  background: none;
  border: none;
  width: 24px;
  height: 24px;
  padding: 4px;
`

export const CoinsButtonSpacer = styled(WalletButton)`
  width: 24px;
  height: 24px;
  padding: 4px;
`

export const CoinsIcon = styled.div`
  position: absolute;
  width: 16px;
  height: 16px;
  top: 4.17%;
  background-image: url(${CoinsIconSVG});
`

export const AddressOrAsset = styled(WalletButton)`
  display: inline;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  color: #4D54D2;
  padding: 0;
`

export const TransactionFeeTooltipTitle = styled.div`
  font-weight: 600;
  letter-spacing: 0.01em;
`

export const TransactionFeeTooltipBody = styled.div`
  font-weight: 400;
  letter-spacing: 0.01em;
`

export const StatusBalanceAndMoreContainer = styled.div`
  display: flex;
  align-self: flex-end;
  flex-direction: row;
  justify-content: flex-end;
  align-items: center;
  width: 50%;
`

export const OrbWrapper = styled(OrbContainer)`
  margin-bottom: 0px;
`
