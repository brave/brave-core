import styled from 'styled-components'

export const Box = styled.div`
  background-color: ${(p) => p.theme.color.background01};
  width: 100%;
  height: 100%;
  position: absolute;
  top: 0;
  bottom: 0;
  right: 0;
  left: 0;
  z-index: 2;
`

export const HeaderBox = styled.section`
  width: 100%;
  padding: 24px 17px 16px 17px;
`

export const TreeBox = styled.section`
  background-color: ${(p) => p.theme.color.background02};
  padding: 10px 17px 10px 17px;
  height: calc(100% - 94px - 96px); /* subtract offset top from height of footer */
  overflow: auto;
  position: relative;
  z-index: 2;
`

export const Footer = styled.section`
  padding: 19px 22px;
  position: absolute;
  bottom: 0;
  width: 100%;
  z-index: 2;

  button {
    width: 100%;

    div {
      display: flex;
      align-items: center;
      justify-content: space-between;
    }

    svg {
      width: 20px;
      height: 20px;
      margin-right: 8px;
    }
  }
`

export const SiteTitleBox = styled.div`
  display: grid;
  grid-template-columns: 24px 1fr;
  grid-gap: 10px;
  align-items: center;
  margin-bottom: 15px;
`

export const SiteTitle = styled.span`
  color: ${(p) => p.theme.color.text01};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 14px;
  font-weight: 500;
`

export const FavIconBox = styled.i`
  width: 22px;
  height: 22px;

  img {
    width: 100%;
    height: auto;
    object-fit: contain;
  }
`

export const BackButton = styled.button`
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 10px 22px;
  width: 100%;

  background-color: transparent;
  border: 1px solid ${(p) => p.theme.color.interactive08};
  border-radius: 48px;

  color: ${(p) => p.theme.color.interactive05};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 13px;
  font-weight: 600;

  svg {
    width: 22px;
    height: 22px;
    margin-right: 8px;
  }

  svg > path {
    color: currentColor;
  }
`

export const Grid = styled.div`
  display: grid;
  grid-template-columns: 20px 2fr 0.5fr;
  grid-gap: 5px;
  align-items: center;
  font-family: ${(p) => p.theme.fontFamily.heading};
  color: ${(p) => p.theme.color.text01};
  font-size: 14px;
  font-weight: 600;
`
