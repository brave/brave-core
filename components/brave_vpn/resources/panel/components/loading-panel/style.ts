import styled from 'styled-components'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  background: ${(p) => p.theme.color.panelBackground};
  overflow: hidden;
  min-height: 260px;
`

export const PanelContent = styled.section`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  padding: 20% 24px 25px 24px;
  z-index: 2;
`

export const LoaderIconBox = styled.div`
  width: 64px;
  height: 64px;
  margin-bottom: 24px;

  svg > path {
    fill: #CA3BB2;
  }
`

export const Title = styled.h1`
  color: ${(p) => p.theme.color.text01};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 14px;
  font-weight: 400;
  letter-spacing: 0.02em;
  line-height: 2;
  margin: 0;
  text-align: center;
`
