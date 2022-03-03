import * as React from 'react'
import { shallow } from 'enzyme'

import { LocaleRestricted } from './locale-restricted'

describe('<LocaleRestricted />', () => {
  it('renders children when user\'s locale is allowed', () => {
    const wrapper = shallow((
      <LocaleRestricted
        allowedLocales={['en-US']}
      >
        <div className="unique" />
      </LocaleRestricted>
    ))
    expect(wrapper.contains(<div className="unique" />)).toEqual(true)
  })

  it('does not render children when user\'s locale is not allowed', () => {
    const wrapper = shallow((
      <LocaleRestricted
        allowedLocales={['fr']}
      >
        <div className="unique" />
      </LocaleRestricted>
    ))
    expect(wrapper.contains(<div className="unique" />)).toEqual(false)
  })
})
