import styled from 'styled-components'

import LedgerLogo from '../../../../../assets/svg-icons/ledger-logo.svg'
import TrezorLogo from '../../../../../assets/svg-icons/trezor-logo.svg'

import { DisclaimerWrapper as DisclaimerWrapperBase } from '../style'

interface StyleProps {
  isSelected: boolean
}

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
  border: ${(p) => (p.isSelected ? `2px solid ${p.theme.color.infoBorder}` : `1px solid ${p.theme.color.disabled}`)};
  background-color: ${(p) => (p.isSelected ? p.theme.color.infoBackground : p.theme.color.background02)};
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

export const ConnectingButton = styled.button`
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  padding: 10px 22px;

  border: 1px solid ${(p) => p.theme.color.interactive08};
  box-sizing: border-box;
  border-radius: 48px;

  // Added manually
  background-color: ${(p) => p.theme.palette.white};
`

export const ConnectingButtonText = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  text-align: center;

  /* Light Theme/Brand/interactive07 */
  color: ${(p) => p.theme.color.interactive07};

  /* Inside Auto Layout */
  flex: none;
  order: 1;
  flex-grow: 0;
  margin: 0px 8px;
`

interface AccountCircleStyleProps {
  orb: string
}

export const HardwareWalletAccountCircle = styled.div<AccountCircleStyleProps>`
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
`

export const HardwareWalletAccountsList = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  padding: 0px;
  width: 100%;

  max-height: 300px;
  overflow-y: auto;

  ::-webkit-scrollbar {
    //width: 0;  /* Remove scrollbar space */
    //background: transparent;  /* Optional: just make scrollbar invisible */
  }
`

export const HardwareWalletAccountListItem = styled.div`
  display: flex;
  flex-direction: row;
  flex-grow: 0;
  margin: 16px 0px;
  width: 100%;
`

export const HardwareWalletAccountListItemColumn = styled.div`
  /* Body Light Theme/14pt Poppins Regular 400 */
  font-family: Poppins;
  font-style: normal;
  font-weight: normal;
  font-size: 14px;
  line-height: 20px;

  display: flex;
  align-items: center;
  letter-spacing: 0.01em;

  /* Light Theme/Text/text01 */
  color: ${(p) => p.theme.color.text01};

  justify-content: space-between;
  width: 100%;
  padding-left: 10px;
`

export const ButtonsContainer = styled.div`
  display: flex;
  flex-direction: row;

  button:first-child {
    margin-right: 10px;
  }
`

export const DisclaimerWrapper = styled(DisclaimerWrapperBase)`
  margin-bottom: 10px;
`

export const SelectWrapper = styled.div`
  margin-bottom: 10px;
  margin-left: -300px;
  width: 250px;
`
