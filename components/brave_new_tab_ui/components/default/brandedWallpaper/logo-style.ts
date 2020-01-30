import styled from 'styled-components'

export const Indicator = styled('div')`
  position: absolute;
  top: 7px;
  right: 7px;
  visibility: hidden;
  width: 20px;
  color: white;
`

export const Anchor = styled('a')`
  position: absolute;
  top: 0;
  right: 0;
  left: 0;
  bottom: 0;
  display: block;
  cursor: pointer;
  &:hover ${Indicator} {
    visibility: visible;
  }
`

export const Image = styled('img')`
  width: 170px;
`
