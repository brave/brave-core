import * as React from 'react'
import { useUnpaddedImageUrl } from '../../../brave_new_tab_ui/components/default/braveNews/useUnpaddedImageUrl'
import styled from 'styled-components'

const Img = styled('img')`
  width: 100%;
`

export default function Image(props: { url: string }) {
  if (true || !props.url) return null

  const src = useUnpaddedImageUrl(props.url)
  return <Img src={src} />
}
