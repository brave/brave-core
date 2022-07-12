import styled from 'styled-components'

export const Box = styled.div`
  background: ${(p) => p.theme.color.background01};
  color: ${(p) => p.theme.color.text01};
  width: 100%;
  height: 100%;
  font-family: ${(p) => p.theme.fontFamily.heading};
  flex: 1 1 auto;
`

export const HeaderBox = styled.div`
  border-bottom: 1px solid #3B3E4F;
  margin-bottom: 24px;
`

export const HeaderContent = styled.div`
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 16px 24px;

  div:first-child {
    flex: 1 1 auto;
  }
`

export const Section = styled.section`
  margin-bottom: 16px;
  padding: 0 24px;

  .title {
    color: ${(p) => p.theme.color.text02};
    font-style: normal;
    font-weight: 600;
    font-size: 14px;
    margin-bottom: 10px;
  }
`

export const SiteName = styled.div`
  font-weight: 600;
`
