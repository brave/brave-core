import styled from 'styled-components'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'
import Refresh from '../../../assets/svg-icons/refresh-icon.svg'

interface StyleProps {
  icon: string | undefined
  isExchange: boolean
  spin: number
  hasError: boolean
}

export const Row = styled.div<Partial<StyleProps>>`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: ${(p) => p.isExchange ? 'flex-start' : 'space-between'};
`

export const FromBalanceText = styled.span<Partial<StyleProps>>`
  font-family: Poppins;
  font-size: 12px;
  margin-top: 4px;
  margin-bottom: ${(p) => p.isExchange ? '0px' : '2px'};
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

export const AssetIcon = styled.div<Partial<StyleProps>>`
  width: 24px;
  height: 24px;
  background: ${(p) => `url(${p.icon})`};
  margin-right: 8px;
  margin-left: 4px;
  background-size: 100%;
`

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
  justify-content: flex-start;
`

export const PresetButton = styled.button`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  padding: 0px;
  margin-right: 8px;
  color: ${(p) => p.theme.color.text01};
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
  font-size: 20px;
  line-height: ${(p) => p.isExchange ? '0px' : '30px'};
  letter-spacing: 0.02em;
  font-weight: 600;
  padding: 0px;
  margin: ${(p) => p.isExchange ? '0px' : '4px 0px'};
  color: ${(p) => p.hasError ? p.theme.color.errorText : p.theme.color.text01};
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
