import * as React from 'react'
import { create } from 'ethereum-blockies'
import { Network, AddNetworkReturnPayload } from '../../../constants/types'
import { reduceAddress } from '../../../utils/reduce-address'
import { NetworkOptions } from '../../../options/network-options'
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
  selectedNetwork: Network
  onCancel: () => void
  onApprove: () => void
  networkPayload: AddNetworkReturnPayload
}

function AllowAddNetworkPanel (props: Props) {
  const {
    selectedNetwork,
    networkPayload,
    onCancel,
    onApprove
  } = props

  const orb = React.useMemo(() => {
    return create({ seed: networkPayload.contractAddress, size: 8, scale: 16 }).toDataURL()
  }, [networkPayload.contractAddress])

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{NetworkOptions[selectedNetwork].abbr}</NetworkText>
        <AddressAndOrb>
          <AddressText>{reduceAddress(networkPayload.contractAddress)}</AddressText>
          <AccountCircle orb={orb} />
        </AddressAndOrb>
      </TopRow>
      <CenterColumn>
        <FavIcon src={`chrome://favicon/size/64@1x/${networkPayload.siteUrl}`} />
        <URLText>{networkPayload.siteUrl}</URLText>
        <PanelTitle>{locale.allowAddNetworkTitle}</PanelTitle>
        <Description>{locale.allowAddNetworkDescription} <DetailsButton>{locale.allowAddNetworkLearnMoreButton}</DetailsButton></Description>
        <MessageBox>
          <MessageBoxColumn>
            <NetworkTitle>{locale.allowAddNetworkName}</NetworkTitle>
            <NetworkDetail>{networkPayload.chainInfo.name}</NetworkDetail>
          </MessageBoxColumn>
          <MessageBoxColumn>
            <NetworkTitle>{locale.allowAddNetworkUrl}</NetworkTitle>
            <NetworkDetail>{networkPayload.chainInfo.url}</NetworkDetail>
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
