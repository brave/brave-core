// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Selectors
import {
  useSafeUISelector
} from '../../../../../../common/hooks/use-safe-selector'
import {
  UISelectors
} from '../../../../../../common/selectors'

// Queries
import {
  useGetSelectedChainQuery,
  useGetSwapSupportedNetworksQuery,
  useSetNetworkMutation
} from '../../../../../../common/slices/api.slice'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Utils
import {
  getLocale
} from '../../../../../../../common/locale'
import {
  reduceNetworkDisplayName
} from '../../../../../../utils/network-utils'

// Components
import {
  ConnectWalletButton
} from '../../buttons/connect-wallet-button/connect-wallet-button'
import {
  SelectTokenOrNetworkButton
} from '../../buttons/select-token-or-network/select-token-or-network'
import {
  NetworkSelector
} from '../network-selector/network-selector'
import {
  AccountModal
} from '../account-modal/account-modal'

// Hooks
import {
  useOnClickOutside
} from '../../../../../../common/hooks/useOnClickOutside'

// Styled Components
import {
  BraveLogo,
  SelectorWrapper,
  Wrapper
} from './header.style'
import {
  Row,
  HorizontalSpacer,
  Text,
  HorizontalDivider,
  HiddenResponsiveRow
} from '../../shared-swap.styles'

export const Header = () => {
  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // Queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: supportedNetworks } = useGetSwapSupportedNetworksQuery()
  const [setNetwork] = useSetNetworkMutation()

  // State
  const [showNetworkSelector, setShowNetworkSelector] =
    React.useState<boolean>(false)
  const [showAccountModal, setShowAccountModal] =
    React.useState<boolean>(false)

  // Refs
  const networkSelectorRef = React.useRef<HTMLDivElement>(null)
  const accountModalRef = React.useRef<HTMLDivElement>(null)

  // Methods
  const onSelectNetwork = React.useCallback(
    async (
      network: BraveWallet.NetworkInfo
    ) => {
      await setNetwork({
        chainId: network.chainId,
        coin: network.coin
      }).unwrap()
      setShowNetworkSelector(false)
    }, [setNetwork])

  // Hooks
  // Click away for network selector
  useOnClickOutside(
    networkSelectorRef,
    () => setShowNetworkSelector(false),
    showNetworkSelector
  )

  // Click away for account modal
  useOnClickOutside(
    accountModalRef,
    () => setShowAccountModal(false),
    showAccountModal
  )

  const isNetworkSupported = React.useMemo(() => {
    return supportedNetworks
      .some(
        supportedNetwork =>
          supportedNetwork.chainId ===
          selectedNetwork?.chainId
      )
  }, [selectedNetwork, supportedNetworks])

  const onClickConnectWalletButton = React.useCallback(async () => {
    setShowAccountModal(prev => !prev)
  }, [])

  return (
    <Wrapper>
      {isPanel ? (
        <HorizontalSpacer size={2} />
      ) : (
        <Row rowHeight='full' verticalAlign='center'>
          <BraveLogo />
          <HiddenResponsiveRow maxWidth={570}>
            <HorizontalDivider
              height={22}
              marginRight={12}
              dividerTheme='darker'
            />
            <Text
              textSize='18px'
              textColor='text02'
              isBold={true}
            >
              {getLocale('braveSwap')}
            </Text>
          </HiddenResponsiveRow>
        </Row>
      )}
      <Row>
        <SelectorWrapper ref={networkSelectorRef}>
          <SelectTokenOrNetworkButton
            onClick={() => setShowNetworkSelector(prev => !prev)}
            text={
              reduceNetworkDisplayName(selectedNetwork?.chainName)
            }
            network={selectedNetwork}
            buttonSize='medium'
            hasBackground={true}
            hasShadow={true}
            isHeader={true}
            networkNotSupported={!isNetworkSupported}
            iconType='network'
          />
          {showNetworkSelector && (
            <NetworkSelector
              isHeader={true}
              onSelectNetwork={onSelectNetwork}
              onClose={() => setShowNetworkSelector(false)} />
          )}
        </SelectorWrapper>
        <HorizontalSpacer size={15} />
        <SelectorWrapper ref={accountModalRef}>
          <ConnectWalletButton onClick={onClickConnectWalletButton} />
          {showAccountModal && (
            <AccountModal
              onHideModal={() => setShowAccountModal(false)}
            />
          )}
        </SelectorWrapper>
      </Row>
    </Wrapper>
  )
}
