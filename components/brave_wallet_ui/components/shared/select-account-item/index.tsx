// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { create } from 'ethereum-blockies'

// types
import { BraveWallet, UserAccountType } from '../../../constants/types'

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
  SwitchAccountIconContainer
} from './style'

export interface Props {
  account?: UserAccountType
  selectedAccount?: UserAccountType
  selectedNetwork?: BraveWallet.NetworkInfo
  onSelectAccount?: () => void
  showTooltips?: boolean
  fullAddress?: boolean
  hideAddress?: boolean
  showSwitchAccountsIcon?: boolean
}

export function SelectAccountItem ({
  account,
  selectedAccount,
  onSelectAccount,
  showTooltips,
  fullAddress,
  selectedNetwork,
  hideAddress,
  showSwitchAccountsIcon: showSwitchAccountsLink
}: Props) {
  // methods
  const onKeyPress = React.useCallback(({ key }: React.KeyboardEvent) => {
    // Invoke for space or enter, just like a regular input or button
    if (onSelectAccount && [' ', 'Enter'].includes(key)) {
      onSelectAccount()
    }
  }, [onSelectAccount])

  // memos / computed
  const accountAddress = account?.address || ''
  const accountName = account?.name || ''

  const orb = React.useMemo(() => {
    return create({ seed: accountAddress.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [accountAddress])

  const PossibleToolTip = React.useMemo(() => {
    return showTooltips ? Tooltip : ({ children }: React.PropsWithChildren<{
      text: string
      isAddress?: boolean
    }>) => <>{children}</>
  }, [showTooltips])

  // render
  return (
    <StyledWrapper onKeyPress={onKeyPress} onClick={onSelectAccount}>
      <LeftSide>
        {!selectedNetwork && <AccountCircle orb={orb} />}
        {selectedNetwork &&
          <IconsWrapper>
            <AccountCircle orb={orb} style={{ width: '36px', height: '36px' }} />
            <NetworkIconWrapper>
              <CreateNetworkIcon size='small' network={selectedNetwork} />
            </NetworkIconWrapper>
          </IconsWrapper>
        }
        <AccountAndAddress>

          <PossibleToolTip
            text={showSwitchAccountsLink
              ? getLocale('braveWalletClickToSwitch')
              : accountName
            }
            isAddress={!showSwitchAccountsLink}
          >
            <Row justifyContent={'flex-start'}>
              <AccountName>{reduceAccountDisplayName(accountName, 22)}</AccountName>
              {showSwitchAccountsLink &&
                <SwitchAccountIconContainer>
                  <SwitchAccountIcon />
                </SwitchAccountIconContainer>
              }
            </Row>
          </PossibleToolTip>

          {!hideAddress &&
            <PossibleToolTip text={accountAddress} isAddress>
              <AccountAddress>{fullAddress
                ? accountAddress
                : reduceAddress(accountAddress)
              }</AccountAddress>
            </PossibleToolTip>
          }

        </AccountAndAddress>
      </LeftSide>
      {accountAddress.toLowerCase() === selectedAccount?.address.toLowerCase() &&
        <BigCheckMark />
      }
    </StyledWrapper>
  )
}

export default SelectAccountItem
