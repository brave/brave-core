import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  margin-left: 8px;
  position: relative;
`

export const DropDown = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-conent: center;
  min-width: 275px;
  padding: 10px 10px 10px 20px;
  background-color: ${(p) => p.theme.color.background02};
  border-radius: 8px;
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  @media (prefers-color-scheme: dark) {
    box-shadow: 0px 0px 16px rgba(0, 0, 0, 0.36);
  }
  border: 1px solid ${(p) => p.theme.color.divider01};
  position: absolute;
  top: 30px;
  left: 0px;
  z-index: 10;
 `
