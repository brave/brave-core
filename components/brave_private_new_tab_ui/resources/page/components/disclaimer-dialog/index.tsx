// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styled from 'styled-components'

const Box = styled.div`
  color: white;
  font-size: 12px;
  line-height: 18px;
  background: rgba(12, 4, 30, 0.2);
  max-width: 285px;
  border-radius: 8px 0px 0px 8px;
  display: inline-block;
  position: relative;

  p {
    padding: 0;
    margin: 0 0 14px 0;

    &:last-child {
      margin-bottom: 0;
    }
  }
`

const ScrollerBox = styled.div`
  /* (100vh - height of dialog header - padding of content box - height of searchbox) / 2 */
  max-height: calc((100vh - 60px - 10px - 175px)/2);
  overflow: auto;
  border-radius: 8px;

  &::-webkit-scrollbar {
    width: 8px;
  }

  &::-webkit-scrollbar-track {
    background: rgba(12, 4, 30, 0.3);
  }

  &::-webkit-scrollbar-thumb {
    background: rgba(255, 255, 255, 0.22);
  }
`

const ContentBox = styled.div`
  padding: 10px 24px 24px;
`

const HeaderBox = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 10px;
  padding: 24px 24px 0;
  position: sticky;
  top: 0;
  width: 100%;
  z-index: 2;

  h1 {
    padding: 0;
    margin: 0;
    font-size: 14px;
    font-weight: 600;
  }
`

const IconButton = styled.button`
  --focus-color: transparent;
  width: 24px;
  height: 24px;
  margin: 0;
  padding: 0;
  outline: 0;
  border: 0;
  border: 2px solid var(--focus-color);
  border-radius: 6px;
  background: transparent;
  cursor: pointer;

  :focus-visible {
    --focus-color: #A0A5EB;
  }
`

const CloseButton = styled(IconButton)`
  svg {
    opacity: 0.7;
  }
`

const InfoButton = styled(IconButton)``

interface Props {
  children: React.ReactNode | React.ReactNode[]
  isOpen: boolean
  onClose?: () => void
  title?: string
}

function DisclaimerDialog (props: Props) {
  const [isOpen, setIsOpen] = React.useState<boolean>(props.isOpen)

  const handleClose = () => {
    setIsOpen(false)
    props.onClose?.()
  }

  const handleOpen = () => setIsOpen(true)

  React.useEffect(() => {
    setIsOpen(props.isOpen)
  }, [props.isOpen])

  if (isOpen) {
    return (
      <Box role="dialog" aria-labelledby="leo-dialog-title">
        <HeaderBox>
          <h1 id="leo-dialog-title">{props?.title}</h1>
          <CloseButton onClick={handleClose} aria-label="Close">
            <svg width="13" height="13" fill="none" xmlns="http://www.w3.org/2000/svg"><path fillRule="evenodd" clipRule="evenodd" d="M2.172 12.26a.867.867 0 1 1-1.226-1.226L5.431 6.55 1.054 2.172A.867.867 0 1 1 2.28.946l4.377 4.378L11.034.946a.867.867 0 0 1 1.226 1.226L7.883 6.549l4.485 4.485a.867.867 0 0 1-1.226 1.226L6.657 7.775 2.172 12.26Z" fill="currentColor"/></svg>
          </CloseButton>
        </HeaderBox>
        <ScrollerBox>
          <ContentBox>
            {props.children}
          </ContentBox>
        </ScrollerBox>
      </Box>
    )
  }

  return (
    <InfoButton onClick={handleOpen} aria-label="Open">
      <svg width="20" height="20" fill="none" xmlns="http://www.w3.org/2000/svg"><path fillRule="evenodd" clipRule="evenodd" d="M17.07 17.072A9.937 9.937 0 0 1 9.996 20a9.936 9.936 0 0 1-7.073-2.93c-3.899-3.898-3.899-10.242 0-14.14A9.936 9.936 0 0 1 9.997 0a9.934 9.934 0 0 1 7.072 2.93A9.93 9.93 0 0 1 20 10c0 2.67-1.04 5.182-2.93 7.072ZM15.89 4.108a8.28 8.28 0 0 0-5.893-2.441 8.28 8.28 0 0 0-5.894 2.44c-3.25 3.25-3.25 8.536 0 11.785a8.28 8.28 0 0 0 5.894 2.441 8.28 8.28 0 0 0 5.894-2.44A8.275 8.275 0 0 0 18.333 10a8.276 8.276 0 0 0-2.442-5.893Zm-4.226 11.725H8.33a.834.834 0 0 1 0-1.666h.834v-3.334H8.33a.834.834 0 0 1 0-1.666h1.667c.46 0 .834.373.834.833v4.167h.834a.833.833 0 1 1 0 1.666ZM9.998 7.5A1.67 1.67 0 0 1 8.33 5.832a1.668 1.668 0 0 1 3.334 0c0 .92-.747 1.668-1.667 1.668Z" fill="#fff" opacity=".65"/></svg>
    </InfoButton>
  )
}

export default DisclaimerDialog
