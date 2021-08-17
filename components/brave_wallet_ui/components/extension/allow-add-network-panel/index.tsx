import * as React from 'react'
import { create } from 'ethereum-blockies'
import { EthereumChain } from '../../../constants/types'
import { reduceAddress } from '../../../utils/reduce-address'
import locale from '../../../constants/locale'
import { NavButton } from '../'

// Styled Components
import {
  MessageBox,
  NetworkTitle,
  MessageBoxColumn,
  DetailsButton,
  ButtonRow,
  FavIcon,
  URLText,
  NetworkDetail
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

export interface Props {
  networkPayload: EthereumChain
  onCancel: () => void
  onApprove: () => void
}

function AllowAddNetworkPanel (props: Props) {
  const {
    networkPayload,
    onCancel,
    onApprove
  } = props
  const rpcUrl = networkPayload.rpcUrls ? (new URL(networkPayload.rpcUrls[0])).hostname : ''
  const iconUrl = networkPayload.iconUrls ? networkPayload.iconUrls[0] : ''
  const orb = React.useMemo(() => {
    return create({ seed: rpcUrl, size: 8, scale: 16 }).toDataURL()
  }, [rpcUrl])

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
        <FavIcon src={iconUrl} />
        <URLText>{rpcUrl}</URLText>
        <PanelTitle>{locale.allowAddNetworkTitle}</PanelTitle>
        <Description>{locale.allowAddNetworkDescription} <DetailsButton>{locale.allowAddNetworkLearnMoreButton}</DetailsButton></Description>
        <MessageBox>
          <MessageBoxColumn>
            <NetworkTitle>{locale.allowAddNetworkName}</NetworkTitle>
            <NetworkDetail>{networkPayload.chainName}</NetworkDetail>
          </MessageBoxColumn>
          <MessageBoxColumn>
            <NetworkTitle>{locale.allowAddNetworkUrl}</NetworkTitle>
            <NetworkDetail>{rpcUrl}</NetworkDetail>
          </MessageBoxColumn>
          <DetailsButton>{locale.allowAddNetworkDetailsButton}</DetailsButton>
        </MessageBox>
      </CenterColumn>
      <ButtonRow>
        <NavButton
          buttonType='secondary'
          text={locale.backupButtonCancel}
          onSubmit={onCancel}
        />
        <NavButton
          buttonType='confirm'
          text={locale.allowAddNetworkApproveButton}
          onSubmit={onApprove}
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default AllowAddNetworkPanel
