import { OpenNewIcon } from 'brave-ui/components/icons'
import * as React from 'react'
import styled from 'styled-components'

const Indicator = styled.span`
  display: none;

  position: absolute;
  top: 8px;
  right: 8px;

  width: 20px;
  height: 20px;
  color: white;
`

const Link = styled.a`
  position: absolute;
  top: 8px;
  left: 8px;
  right: 56px;
  bottom: 8px;
  display: block;
  border: 1px solid red;

  &:hover {
    cursor: pointer;
    ${Indicator} {
      display: block;
    }
  }
`

export default function SponsoredImage(props: { sponsoredImageUrl: string, onClick: () => void }) {
  return (
    <Link href={props.sponsoredImageUrl} rel="noreferrer noopener" onClick={props.onClick}>
      <Indicator>
        <OpenNewIcon />
      </Indicator>
    </Link>
  )
}
