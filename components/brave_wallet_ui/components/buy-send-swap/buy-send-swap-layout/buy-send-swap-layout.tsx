// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet, BuySendSwapTypes } from '../../../constants/types'

// options
import { BuySendSwapOptions } from '../../../options/buy-send-swap-options'

// utils
import { getLocale } from '../../../../common/locale'

// style
import { Tooltip } from '../../shared/tooltip/index'
import {
  StyledWrapper,
  MainContainer,
  MainContainerWrapper,
  ButtonRow,
  TabButton,
  TabButtonText,
  RightDivider,
  LeftDivider,
  HelpCenterText,
  HelpCenterLink
} from './buy-send-swap-layout.style'

export interface Props {
  children?: React.ReactNode
  selectedTab: BuySendSwapTypes
  onChangeTab: (tab: BuySendSwapTypes) => () => void
  isBuyDisabled: boolean
  isSwapDisabled: boolean
  selectedNetwork: BraveWallet.NetworkInfo
}

const getTooltipLocaleKey = (optionId: string): string => {
  if (optionId === 'buy') {
    return 'braveWalletBuyNotSupportedTooltip'
  }

  if (optionId === 'swap') {
    return 'braveWalletSwapNotSupportedTooltip'
  }

  return ''
}

export const BuySendSwapLayout = ({
  children,
  selectedTab,
  onChangeTab,
  isBuyDisabled,
  isSwapDisabled,
  selectedNetwork
}: Props) => {
  return (
    <StyledWrapper>
      <ButtonRow>
        {BuySendSwapOptions().map((option) => {
          const isDisabled =
            isBuyDisabled && option.id === 'buy' ||
            isSwapDisabled && option.id === 'swap'

          return (
            <Tooltip
              maxWidth='94px'
              isVisible={isDisabled}
              key={option.id}
              text={getLocale(getTooltipLocaleKey(option.id)).replace('$1', selectedNetwork.chainName)}
            >
              <TabButton
                isSelected={selectedTab === option.id}
                onClick={onChangeTab(option.id)}
                isDisabled={isDisabled}
                disabled={isDisabled}
              >
                <RightDivider
                  tabID={option.id}
                  selectedTab={selectedTab}
                />
                <LeftDivider
                  tabID={option.id}
                  selectedTab={selectedTab}
                />
                <TabButtonText
                  isSelected={selectedTab === option.id}
                  isDisabled={isDisabled}
                >
                  {option.name}
                </TabButtonText>
              </TabButton>
            </Tooltip>
          )
        })}
      </ButtonRow>
      <MainContainerWrapper>
        <MainContainer selectedTab={selectedTab}>
          {children}
        </MainContainer>
      </MainContainerWrapper>
      <HelpCenterText>
        {getLocale('braveWalletHelpCenterText')}&nbsp;
        <HelpCenterLink target="_blank" href="https://support.brave.com/hc/en-us/articles/4415497656461-Brave-Wallet-FAQ">
          {getLocale('braveWalletHelpCenter')}
        </HelpCenterLink>
      </HelpCenterText>
    </StyledWrapper>
  )
}

export default BuySendSwapLayout
