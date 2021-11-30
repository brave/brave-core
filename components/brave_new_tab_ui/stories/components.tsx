// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import 'svelte'
import * as React from 'react'
import { withKnobs } from '@storybook/addon-knobs'
import ThemeProvider from '../../common/StorybookThemeProvider'
// Components
import LoadingComponent from '../components/loading'
import OutlineButtonComponent from '../components/outlineButton'
// import ButtonSvelte fro../components/svelte/button.sveltelte'
import SvelteButton, { Props } from '../components/svelte/svelte-button'

export default {
  title: 'New Tab',
  decorators: [
    (Story: any) => <ThemeProvider><Story /></ThemeProvider>,
    withKnobs
  ]
}

// function SvelteWrapper (props: {}) {
//   const ref = React.useRef(null)
//   React.useEffect(() => {
//     if (ref.current) {
//       // eslint-disable-next-line no-new
//       new ButtonSvelte({ target: ref.current, data: { buttonText: 'a prop value!' } })
//     }
//   }, [ref.current])
//   return (
//     <div ref={ref}>
//       React:
//     </div>
//   )
// }

let a: Props = { }
console.log(a)

export const Loading = () => (
  <div
    style={{ width: '500px', height: '500px', display: 'flex', alignItems: 'center', justifyContent: 'center' }}
  >
    <SvelteButton buttonText={4} />
    <LoadingComponent />
  </div>
)

export const OutlineButton = () => (
  <div
    style={{ width: '500px', height: '500px', display: 'flex', alignItems: 'center', justifyContent: 'center' }}
  >
    <OutlineButtonComponent>
      Button
    </OutlineButtonComponent>
  </div>
)
