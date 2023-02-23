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
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  display: block;

  &:hover {
    cursor: pointer;
    ${Indicator} {
      display: block;
    }
  }
`

export default function SponsoredImage(props: { sponsoredImageUrl: string }) {
  return (
    <Link href={props.sponsoredImageUrl} rel="noreferrer noopener">
      <Indicator>
        <OpenNewIcon />
      </Indicator>
    </Link>
  )
}
