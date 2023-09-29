// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import {
  BraveWallet,
  SupportedTestNetworks
} from '../../../../../../constants/types'

// api
import { useGetNetworksQuery } from '../../../../../../common/slices/api.slice'

// utils
import Amount from '../../../../../../utils/amount'

// selectors
import { useSafeUISelector } from '../../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../../common/selectors'

// components
import PopupModal from '../../../../popup-modals'
import CreateNetworkIcon from '../../../../../shared/create-network-icon'
import Tooltip from '../../../../../shared/tooltip'

// styles
import * as leo from '@brave/leo/tokens/css'
import {
  Column,
  Row,
  ScrollableColumn,
  VerticalSpace
} from '../../../../../shared/style'
import {
  CategoryTag,
  Name,
  Description,
  Stat,
  StatTitle,
  StatValue,
  NetworksTitle,
  NetworksWrapper,
  ButtonWrapper,
  VisitDappButton,
  LaunchIcon
} from './dapp-details-modal.styles'
import { getLocale } from '../../../../../../../common/locale'

interface Props {
  dapp: BraveWallet.Dapp
  onClose: () => void
}

export const DappDetailsModal = ({ dapp, onClose }: Props) => {
  const {
    name,
    categories,
    description,
    chains,
    uaw,
    transactions,
    volume,
    balance,
    website
  } = dapp

  const activeWallets = new Amount(uaw).abbreviate(2, undefined, 'thousand')
  const formattedTxs = new Amount(transactions).abbreviate(
    2,
    undefined,
    'million'
  )
  const formattedVolume = new Amount(volume).abbreviate(
    2,
    undefined,
    'thousand'
  )
  const formattedBal = new Amount(balance).abbreviate(2, undefined, 'thousand')

  // redux
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // query
  const { data: networks } = useGetNetworksQuery()

  // methods
  const findNetworksByName = React.useCallback(
    (name: string) => {
      return networks
        .filter((network) => !SupportedTestNetworks.includes(network.chainId))
        .filter((network) =>
          network?.chainName.toLowerCase().includes(name.toLowerCase())
        )
    },
    [networks]
  )

  const onVisitDapp = React.useCallback(() => {
    window.open(website, '_blank')
  }, [website])

  // memos
  const dappNetworks = React.useMemo(() => {
    const networks = chains.map((chain) => findNetworksByName(chain)).flat(1)
    return networks
  }, [chains, networks, findNetworksByName])

  return (
    <PopupModal
      onClose={onClose}
      title={getLocale('braveWalletExploreDappsModalTitle')}
      width={isPanel ? '100%' : '436px'}
      borderRadius={16}
      showDivider={false}
    >
      <ScrollableColumn fullWidth justifyContent='center' alignItems='center'>
        <Column
          justifyContent='center'
          alignItems='center'
          padding={
            isPanel
              ? `${leo.spacing['3Xl']} ${leo.spacing.xl} 0 ${leo.spacing.xl}`
              : `${leo.spacing.xl} ${leo.spacing['3Xl']} 0 ${leo.spacing['3Xl']}`
          }
        >
          <div
            style={{
              display: 'flex',
              flexShrink: 0,
              width: '72px',
              height: '72px',
              backgroundColor: 'blue',
              borderRadius: '50%'
            }}
          />
          <VerticalSpace space='8px' />
          <Name>{name}</Name>
          <VerticalSpace space='8px' />
          <Row gap='8px' flexWrap='wrap'>
            {categories.map((category) => (
              <CategoryTag key={category}>{category}</CategoryTag>
            ))}
          </Row>
          <VerticalSpace space='8px' />
          <Description>{description}</Description>
          <VerticalSpace
            space={isPanel ? leo.spacing['2Xl'] : leo.spacing['3Xl']}
          />
          <Column gap='8px' fullWidth>
            <Row gap='8px'>
              <Stat>
                <StatTitle>
                  {getLocale('braveWalletExploreDappActiveWallets')}
                </StatTitle>
                <StatValue>{activeWallets}</StatValue>
              </Stat>
              <Stat>
                <StatTitle>
                  {getLocale('braveWalletExploreDappTransactions')}
                </StatTitle>
                <StatValue>{formattedTxs}</StatValue>
              </Stat>
            </Row>
            <Row gap='8px'>
              <Stat>
                <StatTitle>
                  {getLocale('braveWalletExploreDappVolume')}
                </StatTitle>
                <StatValue>{formattedVolume}</StatValue>
              </Stat>
              <Stat>
                <StatTitle>
                  {getLocale('braveWalletExploreDappBalance')}
                </StatTitle>
                <StatValue>{formattedBal}</StatValue>
              </Stat>
            </Row>
          </Column>
          {dappNetworks.length > 0 ? (
            <>
              <VerticalSpace
                space={isPanel ? leo.spacing['2Xl'] : leo.spacing['3Xl']}
              />
              <NetworksTitle>
                {getLocale('braveWalletExploreDappNetworks')}
              </NetworksTitle>
              <NetworksWrapper>
                {dappNetworks.map((network) => (
                  <Tooltip
                    key={network.chainId}
                    text={network.chainName}
                    position='right'
                    isVisible={true}
                  >
                    <CreateNetworkIcon network={network} size='big' />
                  </Tooltip>
                ))}
              </NetworksWrapper>
            </>
          ) : null}
        </Column>
        <ButtonWrapper isPanel={isPanel}>
          <VisitDappButton onClick={onVisitDapp}>
            {getLocale('braveWalletExploreDappsVisitDapp').replace(
              '$1',
              dapp.name
            )}
            <LaunchIcon />
          </VisitDappButton>
        </ButtonWrapper>
      </ScrollableColumn>
    </PopupModal>
  )
}
