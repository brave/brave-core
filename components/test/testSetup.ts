/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as Enzyme from 'enzyme'
import * as EnzymeAdapter from 'enzyme-adapter-react-16'

import './testPolyfills'

// Setup enzyme's react adapter
Enzyme.configure({ adapter: new EnzymeAdapter() })
