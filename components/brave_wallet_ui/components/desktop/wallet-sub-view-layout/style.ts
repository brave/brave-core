import styled from 'styled-components'

export const StyledWrapper = styled('div') <{ noPadding?: boolean }>`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
  width: 100%;
  height: 100%;
  padding: ${(p) => p.noPadding ? '0px' : '0px 25px'};
  background-color: ${(p) => p.theme.color.background02};
 `
