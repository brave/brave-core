import * as React from 'react'

// Styled Components
import {
  PlaceholderText
} from './style'

export interface Props {
  children?: React.ReactNode
  hideBalances: boolean
  size: 'big' | 'small'
}

function WithHideBalancePlaceholder (props: Props) {
  const {
    children,
    hideBalances,
    size
  } = props

  return (
    <>
      {hideBalances ? (
        <PlaceholderText isBig={size === 'big'} >******</PlaceholderText>
      ) : (
        children
      )}
    </>
  )
}

export default WithHideBalancePlaceholder
