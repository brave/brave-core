import styled from 'styled-components'

export const Box = styled.div`
  display: flex;
  align-items: center;
`

export const Text = styled.span`
  font-family: Poppins;
  font-size: 14px;
  font-weight: 400;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.04em;
`

export const ActiveIndicator = styled.span`
  width: 6px;
  height: 6px;
  margin-right: 8px;
  border-radius: 50%;
  background: ${(p) => p.theme.color.successBorder};
`

export const InActiveIndicator = styled(ActiveIndicator)`
  background: ${(p) => p.theme.color.disabled};
`

export const Loader = styled.span`
  width: 12px;
  height: 12px;
  margin-right: 8px;

  svg>path {
    fill: ${(p) => p.theme.color.text03};
  }
`
