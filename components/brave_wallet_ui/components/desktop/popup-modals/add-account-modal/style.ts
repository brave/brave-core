import styled from 'styled-components'
import LedgerLogo from '../../../../assets/svg-icons/ledger-logo.svg'
import TrezorLogo from '../../../../assets/svg-icons/trezor-logo.svg'
import InfoLogo from '../../../../assets/svg-icons/info-icon.svg'

interface StyleProps {
  isSelected: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  padding: 0px 15px 15px 15px;
  min-height: 320px;
`

export const Input = styled.input`
  outline: none;
  width: 250px;
  background-image: none;
  background-color: ${(p) => p.theme.color.background02};
  box-shadow: none;
  border: ${(p) => `1px solid ${p.theme.color.interactive08}`};
  border-radius: 4px;
  font-family: Poppins;
  font-style: normal;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  padding: 10px;
  margin-bottom: 15px;
  color: ${(p) => p.theme.color.text01};
  ::placeholder {
    font-family: Poppins;
    font-style: normal;
    font-size: 12px;
    letter-spacing: 0.01em;
    color: ${(p) => p.theme.color.text03};
    font-weight: normal;
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

export const SelectWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 250px;
  margin: 15px 0px 15px 0px;
`

export const DisclaimerWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  background-color: ${(p) => p.theme.color.warningBackground};
  border-radius: 16px;
  padding: 10px;
`

export const DisclaimerText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  text-align: center;
  color: ${(p) => p.theme.color.text02};
`

export const HardwareTitle = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  margin-bottom: 25px;
`

export const HardwareButtonRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 260px;
  margin-bottom: 35px;
`

export const HardwareButton = styled.button<StyleProps>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  cursor: pointer;
  outline: none;
  background: none;
  border: ${(p) => p.isSelected ? `2px solid ${p.theme.color.infoBorder}` : `1px solid ${p.theme.color.disabled}`};
  background-color: ${(p) => p.isSelected ? p.theme.color.infoBackground : p.theme.color.background02};
  border-radius: 10px;
  width: 125px;
  height: 55px;
`

export const LedgerIcon = styled.div`
  width: 93px;
  height: 25px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${LedgerLogo});
  mask-image: url(${LedgerLogo});
`

export const TrezorIcon = styled.div`
  width: 105px;
  height: 33px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${TrezorLogo});
  mask-image: url(${TrezorLogo});
`

export const InfoIcon = styled.div`
  width: 12px;
  height: 12px;
  margin-top: 4px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${InfoLogo});
  mask-image: url(${InfoLogo});
`

export const HardwareInfoRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: flex-start;
  margin-bottom: 35px;
`

export const HardwareInfoColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  margin-left: 10px;
`

export const ImportRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  margin-bottom: 15px;
  width: 250px;
`

export const ImportButton = styled.label`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  border-radius: 4px;
  padding: 4px 6px;
  outline: none;
  background-color: transparent;
  border: ${(p) => `1px solid ${p.theme.color.interactive08}`};
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  margin-right: 10px;
  color: ${(p) => p.theme.color.interactive07};
`
