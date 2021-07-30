import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: 100%;
`

export const CategoryRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  width: 100%;
  padding: 8px 0px;
  border-bottom: ${(p) => `1px solid ${p.theme.color.divider01}`};
`

export const CategoryTitle = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 1%;
  color: ${(p) => p.theme.color.text02};
  word-wrap: wrap;
`

export const ButtonRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  width: 100%;
  padding: 12px 0px;
`
