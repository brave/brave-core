import styled from 'styled-components'

export const StyledWrapper = styled('div') <{}>`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-conent: center;
  flex: 1;
  height: 100%;
  padding: 0px 25px;
  background-color: ${(p) => p.theme.color.background02};
 `
