import styled from 'styled-components'

export const Box = styled.button`
  --region-label-color: ${(p) => p.theme.color.text01};
  --svg-color: ${(p) => p.theme.color.text01};
  --border-color: transparent;
  display: flex;
  align-items: center;
  justify-content: space-between;
  min-width: 260px;
  height: 48px;
  border-radius: 38px;
  border: 4px solid var(--border-color);
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  background-color: ${(p) => p.theme.color.background02};
  padding: 0 16px;

  svg {
    width: 18px;
    height: 18px;
  }

  svg>path {
    fill: var(--svg-color);
  }

  &:hover {
    --region-label-color: ${(p) => p.theme.color.interactive05};
    --svg-color: ${(p) => p.theme.color.interactive05};
  }

  &:focus {
    --border-color: #A0A5EB;
  }
`

export const RegionLabel = styled.span`
  font-family: Poppins;
  font-size: 14px;
  font-weight: 400;
  line-height: 20px;
  color: var(--region-label-color);
  letter-spacing: 0.04em;
`
