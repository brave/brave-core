import * as React from 'react'
import { BraveWallet } from '../../../constants/types'
import { getLocale } from '../../../../common/locale'
import { NavButton, PanelTab } from '..'

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
  CenterColumn,
  Description,
  PanelTitle
} from '../shared-panel-styles'

export type tabs = 'network' | 'details'

export interface Props {
  siteOrigin: string
  networkPayload: BraveWallet.EthereumChain
  panelType: 'add' | 'change'
  onCancel: () => void
  onApproveAddNetwork: () => void
  onApproveChangeNetwork: () => void
  onLearnMore: () => void
}

function AllowAddChangeNetworkPanel (props: Props) {
  const {
    siteOrigin,
    networkPayload,
    panelType,
    onCancel,
    onApproveAddNetwork,
    onApproveChangeNetwork,
    onLearnMore
  } = props
  const rpcUrl = networkPayload.rpcUrls.length ? (new URL(networkPayload.rpcUrls[0])).hostname : ''
  const blockUrl = networkPayload.blockExplorerUrls.length ? networkPayload.blockExplorerUrls[0] : ''

  const [selectedTab, setSelectedTab] = React.useState<tabs>('network')
  const onSelectTab = (tab: tabs) => () => {
    setSelectedTab(tab)
  }

  return (
    <StyledWrapper>
      <CenterColumn>
        <FavIcon src={`chrome://favicon/size/64@1x/${siteOrigin}`} />
        <URLText>{siteOrigin}</URLText>
        <PanelTitle>
          {panelType === 'change'
            ? getLocale('braveWalletAllowChangeNetworkTitle')
            : getLocale('braveWalletAllowAddNetworkTitle')
          }
        </PanelTitle>
        <Description>
          {panelType === 'change'
            ? getLocale('braveWalletAllowChangeNetworkDescription')
            : getLocale('braveWalletAllowAddNetworkDescription')}{' '}
          {panelType === 'add' &&
            <DetailsButton
              onClick={onLearnMore}
            >
              {getLocale('braveWalletAllowAddNetworkLearnMoreButton')}
            </DetailsButton>
          }
        </Description>
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
                <NetworkTitle>{getLocale('braveWalletWatchListTokenDecimals')}</NetworkTitle>
                <NetworkDetail>{networkPayload.decimals}</NetworkDetail>
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
          text={
            panelType === 'change'
              ? getLocale('braveWalletAllowChangeNetworkButton')
              : getLocale('braveWalletAllowAddNetworkButton')
          }
          onSubmit={
            panelType === 'add'
              ? onApproveAddNetwork
              : onApproveChangeNetwork
          }
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default AllowAddChangeNetworkPanel
