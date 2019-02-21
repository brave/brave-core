/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { getMockChrome } from './testData'

import { configure } from 'enzyme'
import * as Adapter from 'enzyme-adapter-react-16'


(global as any).chrome = getMockChrome

// not mocking this file make some tests to fail due to non clear reason.
// other action creators didn't need mocking and tests are ok without it.
// since we are not testing action creators ATM it seemed ok to have it mocked
// to unblock other work depending on this test refactor.
// TODO: @cezaraugusto investigate
jest.mock('../app/background/actions/shieldsPanelActions.ts')

configure({ adapter: new Adapter() })
