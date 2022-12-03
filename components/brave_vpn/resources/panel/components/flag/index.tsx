import * as React from 'react'
import styled from 'styled-components'

interface Props {
  countryCode?: string
}

export const IconBox = styled.span`
  width: 24px;
  height: auto;
  display: flex;

  img {
    width: 100%;
    height: 100%;
    object-fit: contain;
  }
`

const COUNTRIES = ['GB', 'US', 'CH', 'ES', 'SG', 'NL', 'JP', 'DE', 'FR', 'CA', 'AU']

function Flag (props: Props) {
  let [url, setUrl] = React.useState(undefined)

  React.useEffect(() => {
    let canceled = false
    const updateSvgUrl = async () => {
      const svgUrl = await import(`../../assets/country-flags/${props.countryCode}.svg`)
      setUrl(svgUrl.default)
    }

    if (COUNTRIES.includes(props?.countryCode ?? '') && !canceled) {
      updateSvgUrl()
    }

    return () => {
      canceled = true
    }
  }, [props?.countryCode])

  if (url) {
    return (
      <IconBox>
        <img src={url} />
      </IconBox>
    )
  }

  return null
}

export default Flag
