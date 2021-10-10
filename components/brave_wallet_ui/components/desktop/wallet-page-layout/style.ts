import styled from 'styled-components'

export const StyledWrapper = styled('div') <{}>`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  background-color: ${(p) => p.theme.color.background02};
  height: 100vh;
  overflow-y: auto;
  overflow-x: hidden;
  width: 100%;
  min-width: 500px;
 `

export const StyledContent = styled('div') <{}>`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: row;
  width: 100%;
  max-width: 1600px;
  padding: 32px 32px 0px 32px;
  @media screen and (max-width: 800px) {
    flex-direction: column;
    align-items: center;
    padding: 32px 0px 0px 0px;
  }
 `
