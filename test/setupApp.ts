/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { getMockChrome } from './testData'

import { configure } from 'enzyme'
import * as Adapter from 'enzyme-adapter-react-16'

(global as any).chrome = getMockChrome

configure({ adapter: new Adapter() })
