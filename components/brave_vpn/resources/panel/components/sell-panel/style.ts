import styled from 'styled-components'
import VPNSvgUrl from '../../assets/svg-icons/vpn-icon.svg'
import checkIconUrl from '../../assets/svg-icons/check-gradient-icon.svg'
import sellGraphicUrl from '../../assets/svg-icons/sell-graphic.svg'
import guardianLogoUrl from '../../assets/svg-icons/guardian-logo.svg'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  background: #381E85;
  position: relative;
`

export const ProductTitle = styled.h1`
  color: #F0F2FF;
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-weight: 600;
  font-size: 20px;
  margin: 0 0 10px 0;
`

export const PoweredBy = styled.div`
  margin-bottom: 16px;
  display: flex;
  align-items: center;

  span {
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-style: normal;
    font-weight: normal;
    font-size: 12px;
    color: #C2C4CF;
  }
`

export const List = styled.ul`
  list-style-type: none;
  padding: 0 32px;
  margin: 0 0 16px 0;

  li {
    color: #F0F2FF;
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-weight: 600;
    font-size: 12px;
    line-height: 18px;
    margin-bottom: 8px;

    &:before {
      content: '';
      display: inline-block;
      width: 16px;
      height: 12px;
      background-image: url(${checkIconUrl});
      background-repeat: no-repeat;
      background-size: cover;
      user-select: none;
      pointer-events: none;
      margin-right: 10px;
    }

    &:last-child {
      margin-bottom: 0;
    }
  }
`

export const PanelContent = styled.section`
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 0 0 25px 0;
  z-index: 2;
`

export const PanelHeader = styled.section`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: 100%;
  padding: 32px 32px 0 32px;
  margin-bottom: 16px;
  box-sizing: border-box;
`

export const SellGraphic = styled.div`
  width: 100%;
  height: 123px;
  background-image: url(${sellGraphicUrl});
  background-repeat: no-repeat;
  background-size: cover;
  position: absolute;
  top: 0;
  user-select: none;
  pointer-events: none;
`

export const MainLogo = styled.div`
  width: 41px;
  height: 48px;
  background-image: url(${VPNSvgUrl});
  background-repeat: no-repeat;
  background-size: cover;
  user-select: none;
  pointer-events: none;
  margin-bottom: 16px;
`

export const GuardianLogo = styled.i`
  width: 75px;
  height: 18px;
  background-image: url(${guardianLogoUrl});
  background-repeat: no-repeat;
  background-size: cover;
  user-select: none;
  pointer-events: none;
  display: inline-block;
  margin-left: 5px;
`
export const ActionArea = styled.div`
  padding: 0 32px;
  width: 100%;
  text-align: center;
  box-sizing: border-box;

  a {
    --color: #F0F2FF;
    color: var(--color);
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-size: 12px;
    text-decoration-line: underline;

    &:focus {
      border: 2px solid var(--color);
      border-radius: 8px;
    }
  }

  button {
    width: 100%;
    margin-bottom: 8px;

    &:first-child {
      backdrop-filter: blur(16px);
      background: rgba(255, 255, 255, 0.24);
      border-color: transparent;

      &:hover {
        background: rgba(255, 255, 255, 0.42);
      }
    }
  }
`
