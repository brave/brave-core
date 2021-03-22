import * as React from 'react'

// Styled Components
import { StyledWrapper, FavIcon, URLText, PanelTitle } from './style'

export interface Props {
  url: string
}

export default class ConnectHeader extends React.PureComponent<Props, {}> {
  render () {
    const { url } = this.props
    return (
      <StyledWrapper>
        {/* FavIcon is a temp fill in until we add logic to fetch site metadata to display fav icons.*/}
        <FavIcon />
        <URLText>{url}</URLText>
        <PanelTitle>Connect With Brave Wallet</PanelTitle>
      </StyledWrapper>
    )
  }
}
