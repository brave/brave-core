import styled from 'styled-components'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'
import Refresh from '../../../assets/svg-icons/refresh-icon.svg'
import ClipboardIcon from '../../../assets/svg-icons/clipboard-icon.svg'
import { BuySendSwapInputType } from './index'
import { AssetIconProps, AssetIconFactory } from '../../shared/style'

interface StyleProps {
  componentType: BuySendSwapInputType
  spin: number
  hasError: boolean
  isSelected: boolean
  isSlippage: boolean
}

export const Row = styled.div<Partial<StyleProps>>`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: ${(p) => p.componentType === 'exchange' ? 'flex-start' : 'space-between'};
`

export const FromBalanceText = styled.span<Partial<StyleProps>>`
  font-family: Poppins;
  font-size: 12px;
  margin-top: 4px;
  margin-bottom: ${(p) => p.componentType === 'exchange' ? '0px' : '2px'};
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`

export const AssetButton = styled.button`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  padding: 0px;
  margin: 0px;
`

// Construct styled-component using JS object instead of string, for editor
// support with custom AssetIconFactory.
//
// Ref: https://styled-components.com/docs/advanced#style-objects
export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '24px',
  height: '24px',
  marginRight: '8px',
  marginLeft: '4px'
})

export const AssetTicker = styled.span`
  font-family: Poppins;
  font-size: 20px;
  line-height: 30px;
  letter-spacing: 0.02em;
  font-weight: 600;
  margin-right: 4px;
  color: ${(p) => p.theme.color.text01};
`

export const CaratDownIcon = styled(CaratStrongDownIcon)`
  width: 12px;
  height: auto;
  color: ${(p) => p.theme.color.text02};
`

export const PresetRow = styled.div`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 4px;
`

export const PresetButton = styled.button<Partial<StyleProps>>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-weight: 600;
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  padding: 2px 0px;
  width: 48px;
  border-radius: 4px;
  background-color: ${(p) => p.isSelected ? p.isSlippage ? p.theme.color.interactive05 : p.theme.color.divider01 : 'none'};
  color: ${(p) => p.isSlippage && p.isSelected ? p.theme.palette.white : p.theme.color.interactive05}};
  @media (prefers-color-scheme: dark) {
    color: ${(p) => p.isSlippage && p.isSelected ? p.theme.palette.white : p.theme.palette.blurple300};
  }
`

export const SlippageInput = styled.input<Partial<StyleProps>>`
  --main-bg-color: ${(p) => p.theme.color.interactive05};
  width: 48px;
  outline: none;
  background-image: none;
  background: ${(p) => p.isSelected ? 'var(--main-bg-color)' : 'none'};
  box-shadow: none;
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  font-weight: 600;
  text-align: center;
  padding: 1px;
  border-radius: 4px;
  border: none;
  border: 1px solid ${(p) => p.isSelected ? p.theme.color.interactive05 : p.theme.color.interactive08};
  color: ${(p) => p.isSelected ? p.theme.palette.white : p.theme.color.text03};
  -webkit-box-shadow: none;
  -moz-box-shadow: none;
  ::placeholder {
    color: ${(p) => p.theme.color.text03};
  }
  :focus {
      outline: none;
  }
  ::-webkit-inner-spin-button {
      -webkit-appearance: none;
      margin: 0;
  }
  ::-webkit-outer-spin-button {
      -webkit-appearance: none;
      margin: 0;
  }
`

export const MarketLimitButton = styled.button`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-family: Poppins;
  font-size: 13px;
  margin-top: 4px;
  letter-spacing: 0.01em;
  padding: 0px;
  color: ${(p) => p.theme.color.interactive05};
`

export const RefreshButton = styled.button`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  width: 18px;
  height: 18px;
  padding: 0px;
  background-color: ${(p) => p.theme.color.divider01};
  border-radius: 100%;
`

export const RefreshIcon = styled.div<Partial<StyleProps>>`
  width: 12px;
  height: 12px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${Refresh});
  mask-image: url(${Refresh});
  animation: ${(p) => p.spin === 1 ? 'spin 1s 1' : 'none'};
  @keyframes spin {
    0% {
      transform: rotate(0deg)
    }
    100% {
      transform: rotate(-360deg)
    }
  }
`

export const Input = styled.input<Partial<StyleProps>>`
  width: 100%;
  outline: none;
  background-image: none;
  background-color: ${(p) => p.theme.color.background02};
  box-shadow: none;
  border: none;
  font-family: Poppins;
  font-size: ${(p) => p.componentType === 'toAddress' ? '14px' : '20px'};
  line-height: ${(p) => p.componentType === 'exchange' ? '0px' : '30px'};
  letter-spacing: 0.02em;
  font-weight: ${(p) => p.componentType === 'toAddress' ? '400' : '600'};
  padding: 0px;
  margin: ${(p) => p.componentType === 'exchange' ? '0px' : '4px 0px'};
  color: ${(p) => p.hasError ? p.theme.color.errorText : p.componentType === 'toAddress' ? p.theme.color.text02 : p.theme.color.text01};
  -webkit-box-shadow: none;
  -moz-box-shadow: none;
  ::placeholder {
    color: ${(p) => p.theme.color.text02};
  }
  :focus {
      outline: none;
  }
  ::-webkit-inner-spin-button {
      -webkit-appearance: none;
      margin: 0;
  }
  ::-webkit-outer-spin-button {
      -webkit-appearance: none;
      margin: 0;
  }
`

export const SelectText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  margin: 2px 0px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`

export const SelectValueText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  margin: 2px 0px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
  margin-right: 4px;
`

export const PasteButton = styled.button`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  width: 18px;
  height: 18px;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  margin: 0px;
  padding: 0px;
  border-radius: 100%;
  background-color: ${(p) => p.theme.color.divider01};
`

export const PasteIcon = styled.div`
  width: 12px;
  height: 12px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${ClipboardIcon});
  mask-image: url(${ClipboardIcon});
  mask-size: 100%;
`

export const WarningText = styled.span`
  font-family: Poppins;
  letter-spacing: 0.01em;
  font-size: 12px;
  color: ${(p) => p.theme.color.errorText};
`
