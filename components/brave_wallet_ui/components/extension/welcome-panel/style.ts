import styled from 'styled-components'
import WelcomeIcon from '../../../assets/svg-icons/onboarding/brave-wallet.svg'
import WelcomeIconDark from '../../../assets/svg-icons/onboarding/brave-wallet-dark.svg'

export const StyledWrapper = styled.div`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: ${(p) => p.theme.color.background01};
`

export const Title = styled.span`
  font-family: Poppins;
  font-size: 15px;
  font-weight: 600;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.04em;
  margin-bottom: 12px;
`

export const Description = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  font-weight: 300;
  color: ${(p) => p.theme.color.text02};
  max-width: 270px;
  text-align: center;
  margin-bottom: 35px;
  letter-spacing: 0.01em;
`

export const PageIcon = styled.div`
  width: 210px;
  height: 160px;
  background: url(${WelcomeIcon});
  background-size: 100%;
  background-repeat: no-repeat;
  margin-bottom: 20px;
  @media (prefers-color-scheme: dark) {
    background: url(${WelcomeIconDark});
  }
`

export const RestoreButton = styled.button`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-family: Poppins;
  font-style: normal;
  font-weight: 500;
  font-size: 13px;
  line-height: 19px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
  margin-top: 12px;
`
