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
  min-width: 1200px;
 `

export const StyledContent = styled('div') <{}>`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: row;
  width: 100%;
  max-width: 1600px;
  padding: 32px 68px 0px 68px;
 `
