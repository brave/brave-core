import * as React from 'react'

// Styled Components
import {
  StyledWrapper,
  AssetBalanceText,
  AssetName,
  BalanceColumn,
  FiatBalanceText,
  NameAndIcon,
  AssetIcon
} from './style'

export interface Props {
  action: () => void
  name: string
  symbol: string
  icon: string
  assetBalance: string
  fiatBalance: string
}

export default class PortfolioAssetItem extends React.PureComponent<Props> {

  navTo = () => () => {
    this.props.action()
  }

  render () {
    const { name, assetBalance, fiatBalance, icon, symbol } = this.props
    return (
      <StyledWrapper onClick={this.navTo()}>
        <NameAndIcon>
          <AssetIcon icon={icon} />
          <AssetName>{name}</AssetName>
        </NameAndIcon>
        <BalanceColumn>
          <FiatBalanceText>${fiatBalance}</FiatBalanceText>
          <AssetBalanceText>{assetBalance} {symbol}</AssetBalanceText>
        </BalanceColumn>
      </StyledWrapper>
    )
  }
}
