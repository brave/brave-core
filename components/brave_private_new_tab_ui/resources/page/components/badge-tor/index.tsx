import * as React from 'react'
import styled, { css } from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'
import { getLocale } from '$web-common/locale'

interface BoxProps {
  isConnected?: boolean
  isLoading?: boolean
}

const Box = styled.div<BoxProps>`
  --bg-color: #BD1531;

  display: inline-flex;
  flex-direction: row;
  align-items: center;
  gap: 12px;
  background: var(--bg-color);
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  color: white;
  font-weight: 600;
  font-size: 14px;
  border-radius: 4px;
  padding: 8px 16px;

  ${p => p.isLoading && css`
    --bg-color: rgba(255, 255, 255, 0.15);
  `}

  ${p => p.isConnected && css`
    --bg-color: #12A378;
  `}

  span {
    width: 22px;
    height: 22px;
  }
`

interface Props {
  isConnected: boolean
  isLoading: boolean
  progress: string
}

function BadgeTor (props: Props) {
  let textElement = getLocale('torStatusDisconnected')
  let iconElement = (
    <svg width="20" height="20" xmlns="http://www.w3.org/2000/svg">
      <path fillRule="evenodd" clipRule="evenodd" d="M10.02 18.181V16.97a6.97 6.97 0 0 0 0-13.938V1.818a8.181 8.181 0 0 1 0 16.363Zm0-4.243a3.94 3.94 0 0 0 0-7.877V4.85a5.15 5.15 0 0 1 0 10.301v-1.212Zm0-6.058a2.12 2.12 0 0 1 0 4.24V7.88ZM0 10c0 5.523 4.477 10 10 10s10-4.477 10-10S15.523 0 10 0 0 4.477 0 10Z" fill="#fff"/>
    </svg>
  )

  const isLoading = Boolean(props.isLoading && props.progress)

  if (props.isConnected) {
    textElement = getLocale('torStatusConnected')
  }

  if (isLoading) {
    textElement = getLocale('torStatusInitializing', { percentage: props.progress })
    iconElement = <LoaderIcon />
  }

  return (
    <Box
      isLoading={isLoading}
      isConnected={props.isConnected}
    >
      <span>{iconElement}</span>
      {textElement}
    </Box>
  )
}

export default BadgeTor
