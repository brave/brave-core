import * as React from 'react'
import { create } from 'ethereum-blockies'
import { EthereumChain } from '../../../constants/types'
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'
import { NavButton, PanelTab } from '../'

// Styled Components
import {
  MessageBox,
  NetworkTitle,
  MessageBoxColumn,
  DetailsButton,
  ButtonRow,
  FavIcon,
  URLText,
  NetworkDetail,
  TabRow
} from './style'

import {
  StyledWrapper,
  AccountCircle,
  AddressAndOrb,
  AddressText,
  CenterColumn,
  Description,
  NetworkText,
  PanelTitle,
  TopRow
} from '../shared-panel-styles'

export type tabs = 'network' | 'details'

export interface Props {
  networkPayload: EthereumChain
  onCancel: () => void
  onApprove: () => void
  onLearnMore: () => void
}

function AllowAddNetworkPanel (props: Props) {
  const {
    networkPayload,
    onCancel,
    onApprove,
    onLearnMore
  } = props
  const rpcUrl = networkPayload.rpcUrls ? (new URL(networkPayload.rpcUrls[0])).hostname : ''
  const blockUrl = networkPayload.blockExplorerUrls ? networkPayload.blockExplorerUrls[0] : ''

  const [selectedTab, setSelectedTab] = React.useState<tabs>('network')
  const orb = React.useMemo(() => {
    return create({ seed: rpcUrl, size: 8, scale: 16 }).toDataURL()
  }, [rpcUrl])

  const onSelectTab = (tab: tabs) => () => {
    setSelectedTab(tab)
  }

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{networkPayload.chainName}</NetworkText>
        <AddressAndOrb>
          <AddressText>{reduceAddress(rpcUrl)}</AddressText>
          <AccountCircle orb={orb} />
        </AddressAndOrb>
      </TopRow>
      <CenterColumn>
        <FavIcon src='' />
        <URLText>{rpcUrl}</URLText>
        <PanelTitle>{getLocale('braveWalletAllowAddNetworkTitle')}</PanelTitle>
        <Description>{getLocale('braveWalletAllowAddNetworkDescription')} <DetailsButton onClick={onLearnMore}>{getLocale('braveWalletAllowAddNetworkLearnMoreButton')}</DetailsButton></Description>
        <TabRow>
          <PanelTab
            isSelected={selectedTab === 'network'}
            onSubmit={onSelectTab('network')}
            text='Network'
          />
          <PanelTab
            isSelected={selectedTab === 'details'}
            onSubmit={onSelectTab('details')}
            text='Details'
          />
        </TabRow>
        <MessageBox>
          <MessageBoxColumn>
            <NetworkTitle>{getLocale('braveWalletAllowAddNetworkName')}</NetworkTitle>
            <NetworkDetail>{networkPayload.chainName}</NetworkDetail>
          </MessageBoxColumn>
          <MessageBoxColumn>
            <NetworkTitle>{getLocale('braveWalletAllowAddNetworkUrl')}</NetworkTitle>
            <NetworkDetail>{rpcUrl}</NetworkDetail>
          </MessageBoxColumn>
          {selectedTab === 'details' &&
            <>
              <MessageBoxColumn>
                <NetworkTitle>{getLocale('braveWalletAllowAddNetworkChainID')}</NetworkTitle>
                <NetworkDetail>{networkPayload.chainId}</NetworkDetail>
              </MessageBoxColumn>
              <MessageBoxColumn>
                <NetworkTitle>{getLocale('braveWalletAllowAddNetworkCurrencySymbol')}</NetworkTitle>
                <NetworkDetail>{networkPayload.symbol}</NetworkDetail>
              </MessageBoxColumn>
              <MessageBoxColumn>
                <NetworkTitle>{getLocale('braveWalletAllowAddNetworkExplorer')}</NetworkTitle>
                <NetworkDetail>{blockUrl}</NetworkDetail>
              </MessageBoxColumn>
            </>
          }
        </MessageBox>
      </CenterColumn>
      <ButtonRow>
        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletBackupButtonCancel')}
          onSubmit={onCancel}
        />
        <NavButton
          buttonType='confirm'
          text={getLocale('braveWalletAllowAddNetworkApproveButton')}
          onSubmit={onApprove}
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default AllowAddNetworkPanel
