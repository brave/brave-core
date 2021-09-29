import styled from 'styled-components'
import WelcomeIcon from '../../../../assets/svg-icons/graphic-wallet-welcome.svg'
import MMIcon from '../../../../assets/svg-icons/meta-mask-icon.svg'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  padding-top: 32px;
`

export const Title = styled.span`
  font-family: Poppins;
  font-size: 20px;
  font-weight: 600;
  line-height: 30px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.02em;
  margin-bottom: 6px;
`

export const Description = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  font-weight: 300;
  color: ${(p) => p.theme.color.text02};
  max-width: 380px;
  text-align: center;
  margin-bottom: 18px;
  letter-spacing: 0.01em;
`

export const PageIcon = styled.div`
  width: 289px;
  height: 169px;
  background: url(${WelcomeIcon});
  margin-bottom: 30px;
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
  margin-bottom: 28px;
`

export const Divider = styled.div`
  width: 180px;
  height: 2px;
  background-color: ${(p) => p.theme.color.divider01};
  margin-bottom: 32px;
`

export const ImportButton = styled.button`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  border-radius: 40px;
  padding: 10px 22px;
  outline: none;
  background-color: transparent;
  border: 1px solid ${(p) => p.theme.color.interactive08};
`

export const ImportButtonText = styled.span`
  font-size: 13px;
  font-weight: 600;
  line-height: 20px;
  color: ${(p) => p.theme.color.interactive07};
`

export const MetaMaskIcon = styled.div`
  width: 27px;
  height: 27px;
  background: url(${MMIcon});
  margin-right: 4px;
`
