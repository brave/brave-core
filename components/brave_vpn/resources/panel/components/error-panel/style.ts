import styled from 'styled-components'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  max-height: 450px;
  background: ${(p) => p.theme.color.panelBackground};
  position: relative;
  overflow: hidden;
`

export const PanelContent = styled.section`
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 48px 24px 16px 24px;
  position: relative;
  z-index: 2;
`

export const ReasonTitle = styled.h1`
  color: ${(p) => p.theme.color.text01};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 18px;
  font-weight: 600;
  letter-spacing: 0.02em;
  line-height: 2.6;
  margin: 0;
  text-align: center;
`

export const ReasonDesc = styled.p`
  color: ${(p) => p.theme.color.text01};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 14px;
  font-weight: 400;
  letter-spacing: 0.02em;
  margin: 0;
  text-align: center;
  padding-bottom: 24px;
`

export const IconBox = styled.div`
  width: 62px;
  height: 62px;
`

export const ActionArea = styled.div`
  width: 100%;

  button {
    width: 100%;

    &:first-child {
      margin-bottom: 20px;
    }
  }
`
