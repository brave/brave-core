// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

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
import {
  CreateAccountIcon
} from '../../shared/create-account-icon/create-account-icon'

// Styled Components
import {
  StyledWrapper,
  CollapseButton,
  CollapseIcon
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
  hideBalance?: boolean
  children?: React.ReactNode,
  hasBorder?: boolean
}

export const AssetGroupContainer = (props: Props) => {
  const {
    balance,
    hideBalance,
    account,
    isDisabled,
    network,
    children,
    hasBorder = true
  } = props

  // Selectors
  const hidePortfolioBalances =
    useSafeWalletSelector(WalletSelectors.hidePortfolioBalances)

  // State
  const [isCollapsed, setIsCollapsed] = React.useState<boolean>(false)

  return (
    <StyledWrapper
      fullWidth={true}
      hasBorder={hasBorder}
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
            <CreateAccountIcon
              size='small'
              address={account.address}
              accountKind={account.accountId.kind}
              marginRight={16}
            />
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
          {balance !== '' && !hideBalance ? (
            <Text
              textSize='12px'
              isBold={false}
              textColor='text02'
            >
              {hidePortfolioBalances ? '******' : balance}
            </Text>
          ) : (
            <>
              {!hideBalance && <LoadingSkeleton width={60} height={14} />}
            </>
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
