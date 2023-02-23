import * as React from 'react'
import styled from 'styled-components'

const Link = styled.a`
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  border: 1px solid red;
  display: block;

  &:hover {
    cursor: pointer;
  }
`

export default function SponsoredImage(props: { sponsoredImageUrl: string }) {
  return <Link href={props.sponsoredImageUrl} />
}
