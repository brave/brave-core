import * as React from 'react'

// Styled Components
import {
  StyledWrapper,
  ProfileCircle,
  URLText,
  PanelTitle,
  Details
} from './style'

export interface Props {
  detailText: string
}

export default class ConnectHeader extends React.PureComponent<Props, {}> {
  render () {
    const { detailText } = this.props
    return (
      <StyledWrapper>
        <ProfileCircle />
        <URLText>{detailText}</URLText>
        <PanelTitle>Connect With Brave Wallet</PanelTitle>
        <Details>Select accounts(s)</Details>
      </StyledWrapper>
    )
  }
}
