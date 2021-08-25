import * as React from 'react'
import { reduceAddress } from '../../../utils/reduce-address'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { create } from 'ethereum-blockies'
import { Tooltip } from '../../shared'
import locale from '../../../constants/locale'

// Styled Components
import {
  StyledWrapper,
  AssetBalanceText,
  AccountName,
  AccountAddress,
  AccountAndAddress,
  BalanceColumn,
  FiatBalanceText,
  NameAndIcon,
  AccountCircle,
  MoreButton,
  MoreIcon,
  RightSide
} from './style'

export interface Props {
  action: () => void
  address: string
  fiatBalance: string
  assetBalance: string
  assetTicker: string
  name: string
}

const PortfolioAccountItem = (props: Props) => {
  const { address, name, assetBalance, fiatBalance, assetTicker, action } = props

  const onCopyToClipboard = async () => {
    await copyToClipboard(address)
  }

  const orb = React.useMemo(() => {
    return create({ seed: address, size: 8, scale: 16 }).toDataURL()
  }, [address])

  return (
    <StyledWrapper>
      <NameAndIcon>
        <AccountCircle orb={orb} />
        <Tooltip text={locale.toolTipCopyToClipboard}>
          <AccountAndAddress onClick={onCopyToClipboard}>
            <AccountName>{name}</AccountName>
            <AccountAddress>{reduceAddress(address)}</AccountAddress>
          </AccountAndAddress>
        </Tooltip>
      </NameAndIcon>
      <RightSide>
        <BalanceColumn>
          <FiatBalanceText>${fiatBalance}</FiatBalanceText>
          <AssetBalanceText>{assetBalance} {assetTicker}</AssetBalanceText>
        </BalanceColumn>
        <MoreButton onClick={action}>
          <MoreIcon />
        </MoreButton>
      </RightSide>
    </StyledWrapper>
  )
}

export default PortfolioAccountItem
