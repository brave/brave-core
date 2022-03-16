import styled from 'styled-components'
import { AssetIconProps, AssetIconFactory, WalletButton } from '../../shared/style'

interface StyleProps {
  disabled: boolean
}

export const StyledWrapper = styled(WalletButton) <Partial<StyleProps>>`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  margin: 8px 0px;
  cursor: ${(p) => p.disabled ? 'default' : 'pointer'};
  outline: none;
  background: none;
  border: none;
`

export const NameAndIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`

export const AssetName = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
`

export const BalanceColumn = styled.div`
  display: flex;
  align-items: flex-end;
  justify-content: center;
  flex-direction: column;
`

export const FiatBalanceText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
`

export const AssetBalanceText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`

// Construct styled-component using JS object instead of string, for editor
// support with custom AssetIconFactory.
//
// Ref: https://styled-components.com/docs/advanced#style-objects
export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto'
})

export const IconsWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: relative;
  margin-right: 6px;
`

export const NetworkIconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: absolute;
  bottom: 0px;
  right: 4px;
  background-color: ${(p) => p.theme.color.background02};
  border-radius: 100%;
  padding: 2px;
`

export const NameColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
`

export const Spacer = styled.div`
  display: flex;
  height: 4px;
`
