import styled from 'styled-components'

interface StyleProps {
  orb: string
}

export const StyledWrapper = styled.button`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  margin: 10px 0px;
  padding: 0px;
`

export const NetworkName = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
`

export const AccountCircle = styled.div<StyleProps>`
  width: 10px;
  height: 10px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-right: 14px;
`
