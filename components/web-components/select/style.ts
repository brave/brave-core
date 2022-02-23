import styled from 'styled-components'

export const SelectBox = styled.div`
  position: relative;

  svg {
    position: absolute;
    right: 10px;
    top: 40%;
    pointer-events: none;

    path {
      fill: ${(p) => p.theme.color.text02};
    }
  }
`

export const Select = styled.select`
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 12px;
  line-height: 1.2;
  padding: 9px calc(8px + 20px) 9px 8px; /* Offset left of svg icon */
  color: ${(p) => p.theme.color.text01};
  background-color: ${(p) => p.theme.color.background01};
  border-radius: 4px;
  border: 0;
  width: 100%;
  appearance: none;

  &:focus-visible {
    outline: 0;
    border-color: ${(p) => p.theme.color.focusBorder};
  }
`
