import * as React from 'react'
import { reduceAddress } from '../../../utils/reduce-address'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { create } from 'ethereum-blockies'
import { Tooltip } from '../../shared'
import locale from '../../../constants/locale'
import { EthereumChain } from '../../../constants/types'
import { TransactionPopup } from '../'
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
  address: string
  fiatBalance: string
  assetBalance: string
  assetTicker: string
  selectedNetwork: EthereumChain
  name: string
}

const PortfolioAccountItem = (props: Props) => {
  const { address, name, assetBalance, fiatBalance, assetTicker, selectedNetwork } = props
  const [showAccountPopup, setShowAccountPopup] = React.useState<boolean>(false)
  const onCopyToClipboard = async () => {
    await copyToClipboard(address)
  }

  const orb = React.useMemo(() => {
    return create({ seed: address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [address])

  const onClickViewOnBlockExplorer = () => {
    const exporerURL = selectedNetwork.blockExplorerUrls[0]
    if (exporerURL && address) {
      const url = `${exporerURL}/address/${address}`
      window.open(url, '_blank')
    } else {
      alert(locale.transactionExplorerMissing)
    }
  }

  const onShowTransactionPopup = () => {
    setShowAccountPopup(true)
  }

  const onHideTransactionPopup = () => {
    if (showAccountPopup) {
      setShowAccountPopup(false)
    }
  }

  return (
    <StyledWrapper onClick={onHideTransactionPopup}>
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
        <MoreButton onClick={onShowTransactionPopup}>
          <MoreIcon />
        </MoreButton>
        {showAccountPopup &&
          <TransactionPopup onClickView={onClickViewOnBlockExplorer} />
        }
      </RightSide>
    </StyledWrapper>
  )
}

export default PortfolioAccountItem
