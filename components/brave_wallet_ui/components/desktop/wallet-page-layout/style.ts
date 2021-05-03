import styled from 'styled-components'

export const StyledWrapper = styled('div') <{}>`
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: ${(p) => p.theme.color.background02};
  height: 100vh;
  width: 100%;
  min-width: 1200px;
 `

export const StyledContent = styled('div') <{}>`
  display: flex;
  align-items: flex-start;
  justify-conent: flex-start;
  flex-direction: row;
  width: 100%;
  max-width: 1600px;
  height: 100%;
  padding: 32px 68px 0px 68px;
 `
