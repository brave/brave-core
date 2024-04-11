// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styled, { css } from 'styled-components'

import getPanelBrowserAPI from '../../api/panel_browser_api'

interface Props extends React.PropsWithChildren {
  isExpanded: boolean
}

interface ScrollBoxProps {
  scrollHeight?: number | null
}

const ScrollBox = styled.div<ScrollBoxProps>`
  --height: auto;

  overflow: auto;
  max-height: var(--height);

  ${p => p.scrollHeight && css`
    --height: ${p.scrollHeight}px;
  `}

  &::-webkit-scrollbar {
    width: 8px;
  }

  &::-webkit-scrollbar-track {
    background: ${p => p.theme.color.subtleBackground};
  }

  &::-webkit-scrollbar-thumb {
    background: ${p => p.theme.color.subtle};
  }
`

function getWindowInnerHeight (): Promise<number> {
  // "visibility" change triggers multiple (3) times with ~30-80ms delay between each trigger.
  // A safe number of 100 is chosen to wait for the browser to complete paint
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve(window.innerHeight)
    }, 100)
  })
}

function Scroller (props: Props) {
  const ref = React.useRef<HTMLDivElement>(null)
  const [rect, setRect] = React.useState<DOMRectReadOnly | undefined>(undefined)

  let scrollHeight: number | null = null

  if (rect && rect.bottom >= window.screen.height) {
    const diff = rect.bottom - window.screen.height
    if (ref.current) {
      scrollHeight = ref.current.clientHeight - diff
    }
  }

  const updatePanelRect = async () => {
    const { vec } = await getPanelBrowserAPI().panelHandler.getPosition()
    const windowInnerHeight = await getWindowInnerHeight()

    // We read width/height from window object because
    // the browser will reflow and always return
    // accurate values
    const rectReadOnly = new DOMRectReadOnly(
      vec.x, vec.y,
      window.innerWidth, windowInnerHeight)
    setRect(rectReadOnly)
  }

  // We use layoutEffect to avoid flickrs when scrollbar is painted or removed
  React.useLayoutEffect(() => {
    if (props.isExpanded) {
      updatePanelRect()
    }
  }, [props.isExpanded])

  React.useLayoutEffect(() => {
    const onVisibilityChange = () => {
      if ((document.visibilityState === 'visible') && props.isExpanded) {
        updatePanelRect()
      }
    }

    document.addEventListener('visibilitychange', onVisibilityChange)

    return () => {
      document.removeEventListener('visibilitychange', onVisibilityChange)
    }
  }, [])

  return (
    <ScrollBox ref={ref} scrollHeight={scrollHeight}>
      {props.children}
    </ScrollBox>
  )
}

export default Scroller
