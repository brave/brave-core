/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 import * as React from 'react'
 import { storiesOf } from '@storybook/react'
 import { withKnobs } from '@storybook/addon-knobs'
 
 // Components
 
 storiesOf('New Tab/Widgets', module)
   .addDecorator(withKnobs)
   .add('Generic', () => {
     return (
       <h1>Test</h1>
     )
   })