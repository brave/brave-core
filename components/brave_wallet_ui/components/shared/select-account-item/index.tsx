// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// types
import { BraveWallet } from '../../../constants/types'

// utils
import { getLocale } from '../../../../common/locale'
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'

// components
import { Tooltip } from '../tooltip/index'
import { CreateNetworkIcon } from '../create-network-icon/index'

// style
import {
  IconsWrapper,
  NetworkIconWrapper,
  SwitchAccountIcon,
  Row
} from '../style'
import {
  StyledWrapper,
  AccountAddress,
  AccountAndAddress,
  AccountCircle,
  AccountName,
  LeftSide,
  BigCheckMark,
  SwitchAccountIconContainer,
  CaratDown
} from './style'

// hooks
import { useAccountOrb } from '../../../common/hooks/use-orb'

export interface Props {
  account?: BraveWallet.AccountInfo
  isSelected?: boolean
  selectedNetwork?: BraveWallet.NetworkInfo
  onSelectAccount?: () => void
  showTooltips?: boolean
  fullAddress?: boolean
  hideAddress?: boolean
  showSwitchAccountsIcon?: boolean
  isV2?: boolean
}

export function SelectAccountItem({
  account,
  isSelected,
  onSelectAccount,
  showTooltips,
  fullAddress,
  selectedNetwork,
  hideAddress,
  showSwitchAccountsIcon: showSwitchAccountsLink,
  isV2
}: Props) {
  // methods
  const onKeyPress = React.useCallback(
    ({ key }: React.KeyboardEvent) => {
      // Invoke for space or enter, just like a regular input or button
      if (onSelectAccount && [' ', 'Enter'].includes(key)) {
        onSelectAccount()
      }
    },
    [onSelectAccount]
  )

  // memos / computed
  const accountAddress = account?.address || ''
  const accountName = account?.name || ''

  const orb = useAccountOrb(account)

  const PossibleToolTip = React.useMemo(() => {
    return showTooltips
      ? Tooltip
      : ({
          children
        }: React.PropsWithChildren<{
          text: string
          isAddress?: boolean
        }>) => <>{children}</>
  }, [showTooltips])

  // render
  return (
    <StyledWrapper
      onKeyPress={onKeyPress}
      onClick={onSelectAccount}
      isV2={isV2}
    >
      <LeftSide>
        {!selectedNetwork && <AccountCircle orb={orb} />}
        {selectedNetwork && (
          <IconsWrapper>
            <AccountCircle
              orb={orb}
              style={{ width: '36px', height: '36px' }}
            />
            <NetworkIconWrapper>
              <CreateNetworkIcon
                size='small'
                network={selectedNetwork}
              />
            </NetworkIconWrapper>
          </IconsWrapper>
        )}
        <AccountAndAddress>
          <PossibleToolTip
            text={
              showSwitchAccountsLink
                ? getLocale('braveWalletClickToSwitch')
                : accountName
            }
            isAddress={!showSwitchAccountsLink}
          >
            <Row justifyContent={'flex-start'}>
              <AccountName>
                {reduceAccountDisplayName(accountName, 22)}
              </AccountName>
              {showSwitchAccountsLink && !isV2 && (
                <SwitchAccountIconContainer>
                  <SwitchAccountIcon />
                </SwitchAccountIconContainer>
              )}
            </Row>
          </PossibleToolTip>

          {!hideAddress && (
            <PossibleToolTip
              text={accountAddress}
              isAddress
            >
              <AccountAddress>
                {fullAddress ? accountAddress : reduceAddress(accountAddress)}
              </AccountAddress>
            </PossibleToolTip>
          )}
        </AccountAndAddress>
      </LeftSide>
      {isSelected && <BigCheckMark />}
      {isV2 && <CaratDown />}
    </StyledWrapper>
  )
}

export default SelectAccountItem
