// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import { useDispatch } from 'react-redux'

// Actions
import {
  WalletActions
} from '../../../common/actions'

// Selectors
import {
  useUnsafeWalletSelector
} from '../../../common/hooks/use-safe-selector'
import {
  WalletSelectors
} from '../../../common/selectors'

// Types
import {
  DAppConnectionOptionsType
} from './dapp-connection-settings'
import {
  BraveWallet
} from '../../../constants/types'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetSelectedChainQuery
} from '../../../common/slices/api.slice'
import {
  useSelectedAccountQuery
} from '../../../common/slices/api.slice.extra'

// Utils
import {
  reduceAddress
} from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'

// Components
import {
  CreateAccountIcon
} from '../../shared/create-account-icon/create-account-icon'
import {
  CreateNetworkIcon,
  CreateSiteOrigin,
  LoadingSkeleton
} from '../../shared'

// Styled Components
import {
  ConnectedIcon,
  ConnectedText,
  SectionButton,
  SectionRow,
  NameText,
  DescriptionText,
  ButtonIcon,
  FavIcon
} from './dapp-connection-settings.style'
import {
  VerticalSpace,
  HorizontalSpace,
  Column,
  Row
} from '../../shared/style'

interface Props {
  isConnected: boolean
  isPermissionDenied: boolean
  onSelectOption: (option: DAppConnectionOptionsType) => void
  getAccountsFiatValue: (account: BraveWallet.AccountInfo) => Amount
}

export const DAppConnectionMain = (props: Props) => {
  const {
    isConnected,
    isPermissionDenied,
    onSelectOption,
    getAccountsFiatValue
  } = props
  // Redux
  const dispatch = useDispatch()

  // Selectors
  const activeOrigin =
    useUnsafeWalletSelector(WalletSelectors.activeOrigin)

  // Queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: selectedAccount } = useSelectedAccountQuery()
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  // Memos
  const connectionStatusText = React.useMemo((): string => {
    if (isPermissionDenied) {
      return getLocale('braveWalletPanelBlocked')
    }
    return isConnected
      ? getLocale('braveWalletPanelConnected')
      : getLocale('braveWalletNotConnected')
  }, [isConnected, isPermissionDenied])

  const selectedAccountFiatValue = React.useMemo(() => {
    if (!selectedAccount) {
      return Amount.empty()
    }
    return getAccountsFiatValue(selectedAccount)
  }, [
    getAccountsFiatValue,
    selectedAccount
  ])

  const showConnectOptions = React.useMemo(() => {
    if (activeOrigin.originSpec.startsWith('chrome')) {
      return false
    }
    return !isPermissionDenied
  }, [isPermissionDenied, activeOrigin.originSpec])

  // Methods
  const onClickConnect = React.useCallback(() => {
    if (!selectedAccount) {
      return
    }
    dispatch(
      WalletActions
        .addSitePermission(
          { accountId: selectedAccount.accountId }
        )
    )
  }, [selectedAccount])

  const onClickDisconnect = React.useCallback(() => {
    if (!selectedAccount) {
      return
    }
    dispatch(
      WalletActions
        .removeSitePermission(
          { accountId: selectedAccount.accountId }
        )
    )
  }, [selectedAccount])

  return (
    <>
      <Row
        justifyContent='flex-start'
        marginBottom={16}
      >
        <ConnectedIcon
          name={
            isConnected
              ? 'check-circle-filled'
              : 'social-dribbble'
          }
          dappConnected={isConnected}
          size='16px'
        />
        <HorizontalSpace space='8px' />
        <ConnectedText
          dappConnected={isConnected}
          textSize='16px'
          isBold={true}
        >
          {connectionStatusText}
        </ConnectedText>
      </Row>
      <SectionRow>
        <Row
          width='unset'
        >
          <FavIcon
            size='32px'
            marginRight='16px'
            src={
              `chrome://favicon/size/64@1x/${activeOrigin.originSpec}`
            }
          />
          <Column
            alignItems='flex-start'
            padding='0px 8px 0px 0px'
          >
            <NameText
              textSize='14px'
              isBold={true}
            >
              {activeOrigin.eTldPlusOne}
            </NameText>
            <DescriptionText
              textSize='12px'
              isBold={false}
            >
              <CreateSiteOrigin
                originSpec={activeOrigin.originSpec}
                eTldPlusOne={activeOrigin.eTldPlusOne}
              />
            </DescriptionText>
          </Column>
        </Row>
        {showConnectOptions &&
          <Row
            width='unset'
          >
            <Button
              size='small'
              onClick={
                isConnected
                  ? onClickDisconnect
                  : onClickConnect
              }
            >
              {
                isConnected
                  ? getLocale('braveWalletSitePermissionsDisconnect')
                  : getLocale('braveWalletAddAccountConnect')
              }
            </Button>
          </Row>
        }
      </SectionRow>
      {showConnectOptions &&
        <>
          <VerticalSpace space='8px' />
          <SectionButton
            onClick={() => onSelectOption('accounts')}
          >
            <Row
              width='unset'
            >
              {selectedAccount &&
                <CreateAccountIcon
                  size='medium'
                  account={selectedAccount}
                  marginRight={16}
                />
              }
              <Column
                alignItems='flex-start'
              >
                <NameText
                  textSize='14px'
                  isBold={true}
                >
                  {selectedAccount?.name}
                </NameText>
                <DescriptionText
                  textSize='12px'
                  isBold={false}
                >
                  {
                    reduceAddress(
                      selectedAccount?.accountId?.address ?? '')
                  }
                </DescriptionText>
                {selectedAccountFiatValue.isUndefined() ? (
                  <>
                    <VerticalSpace space='3px' />
                    <LoadingSkeleton width={60} height={12} />
                    <VerticalSpace space='3px' />
                  </>
                ) : (
                  <DescriptionText
                    textSize='12px'
                    isBold={false}
                  >
                    {
                      selectedAccountFiatValue
                        .formatAsFiat(defaultFiatCurrency)
                    }
                  </DescriptionText>
                )}
              </Column>
            </Row>
            <ButtonIcon />
          </SectionButton>
        </>
      }
      <VerticalSpace space='8px' />
      <SectionButton
        onClick={() => onSelectOption('networks')}
        data-test-id='select-network-button'
      >
        <Row
          width='unset'
          padding='8px 0px'
        >
          <CreateNetworkIcon
            network={selectedNetwork}
            marginRight={16}
            size='huge'
          />
          <Column
            alignItems='flex-start'
          >
            <DescriptionText
              textSize='12px'
              isBold={false}
            >
              {getLocale('braveWalletTransactionDetailNetwork')}
            </DescriptionText>
            <NameText
              textSize='14px'
              isBold={false}
            >
              {selectedNetwork?.chainName}
            </NameText>
          </Column>
        </Row>
        <ButtonIcon />
      </SectionButton>
    </>
  )
}
