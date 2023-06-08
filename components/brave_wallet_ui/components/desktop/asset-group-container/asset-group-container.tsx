// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { create } from 'ethereum-blockies'

// Selectors
import {
  useSafeWalletSelector,
} from '../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../common/selectors'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'

// Types
import { BraveWallet } from '../../../constants/types'

// Components
import {
  CreateNetworkIcon,
  LoadingSkeleton
} from '../../shared'

// Styled Components
import {
  StyledWrapper,
  CollapseButton,
  CollapseIcon,
  AccountSquare
} from './asset-group-container.style'
import {
  Row,
  Column,
  Text,
  VerticalDivider,
  HorizontalSpace
} from '../../shared/style'

interface Props {
  network?: BraveWallet.NetworkInfo | undefined
  account?: BraveWallet.AccountInfo | undefined
  isDisabled?: boolean
  balance: string
  children?: React.ReactNode
}

export const AssetGroupContainer = (props: Props) => {
  const {
    balance,
    account,
    isDisabled,
    network,
    children
  } = props

  // Selectors
  const hidePortfolioBalances =
    useSafeWalletSelector(WalletSelectors.hidePortfolioBalances)

  // State
  const [isCollapsed, setIsCollapsed] = React.useState<boolean>(false)

  // Memos
  const orb = React.useMemo(() => {
    return create(
      {
        seed: account?.address.toLowerCase(),
        size: 8,
        scale: 16
      }
    ).toDataURL()
  }, [account?.address])

  return (
    <StyledWrapper
      fullWidth={true}
    >
      <CollapseButton
        onClick={() => setIsCollapsed(prev => !prev)}
        disabled={isDisabled}
      >
        {network &&
          <Row
            width='unset'
          >
            <CreateNetworkIcon
              network={network}
              marginRight={16}
              size='big'
            />
            <Text
              textSize='14px'
              isBold={true}
              textColor='text01'
            >
              {network.chainName}
            </Text>
          </Row>
        }

        {account &&
          <Row
            width='unset'
          >
            <AccountSquare orb={orb} />
            <Text
              textSize='14px'
              isBold={true}
              textColor='text01'
            >
              {account.name}
            </Text>
            <HorizontalSpace space='8px' />
            <Text
              textSize='12px'
              isBold={false}
              textColor='text02'
            >
              {reduceAddress(account.address)}
            </Text>
          </Row>
        }

        <Row
          width='unset'
        >
          {balance !== '' ? (
            <Text
              textSize='12px'
              isBold={false}
              textColor='text02'
            >
              {hidePortfolioBalances ? '******' : balance}
            </Text>
          ) : (
            <LoadingSkeleton width={60} height={14} />
          )}

          {!isDisabled &&
            <CollapseIcon
              isCollapsed={isCollapsed}
              name='carat-down'
            />
          }
        </Row>
      </CollapseButton>

      {!isCollapsed && !isDisabled &&
        <Column
          fullWidth={true}
        >
          <Row
            padding='0px 8px'
          >
            <VerticalDivider />
          </Row>
          {children}
        </Column>
      }
    </StyledWrapper>

  )
}
