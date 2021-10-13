/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LoadingIcon } from '../icons/loading_icon'

import * as style from './async_button.style'

interface Props {
  onClick: () => void
  children: React.ReactNode
}

export function AsyncButton (props: Props) {
  const [clicked, setClicked] = React.useState(false)

  // When the button is clicked, disable the button and show a loading indicator
  // for a fixed amount of time.
  React.useEffect(() => {
    if (clicked) {
      const timer = setTimeout(() => setClicked(false), 3000)
      return () => clearTimeout(timer)
    }
    return () => undefined
  }, [clicked])

  function onClick () {
    setClicked(true)
    props.onClick()
  }

  return (
    <style.root>
      <button onClick={onClick} disabled={clicked}>
        <style.content>{props.children}</style.content>
        <style.spinner><LoadingIcon /></style.spinner>
      </button>
    </style.root>
  )
}
