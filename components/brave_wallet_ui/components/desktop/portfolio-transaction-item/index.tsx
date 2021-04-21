import * as React from 'react'
import { reduceAddress } from '../../../utils/reduce-address'
import { create } from 'ethereum-blockies'

// Styled Components
import {
  StyledWrapper,
  DetailText,
  AddressText,
  DetailRow,
  FromCircle,
  ToCircle,
  MoreButton,
  MoreIcon
} from './style'

export interface Props {
  action: () => void
  to: string
  from: string
  ticker: string
  amount: number
}

const PortfolioTransactionItem = (props: Props) => {
  const { to, from, amount, ticker, action } = props

  const fromOrb = React.useMemo(() => {
    return create({ seed: from, size: 8, scale: 16 }).toDataURL()
  }, [from])

  const toOrb = React.useMemo(() => {
    return create({ seed: to, size: 8, scale: 16 }).toDataURL()
  }, [to])

  return (
    <StyledWrapper>
      <DetailRow>
        <FromCircle orb={fromOrb} />
        <ToCircle orb={toOrb} />
        <DetailText>{amount} {ticker} from </DetailText>
        <AddressText>{reduceAddress(from ? from : '')}</AddressText>
        <DetailText> to </DetailText>
        <AddressText>{reduceAddress(to ? to : '')}</AddressText>
      </DetailRow>
      <MoreButton onClick={action}>
        <MoreIcon />
      </MoreButton>
    </StyledWrapper>
  )
}

export default PortfolioTransactionItem
