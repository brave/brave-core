/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useDispatch } from 'react-redux';

// Feature-specific components
import { Content, Title, Paragraph } from '../../components'
import { Checkbox } from 'brave-ui/components'

// Utils
import { getLocale } from '../../../common/locale'
import { loadTimeData } from '../../../common/loadTimeData'
import { setP3AEnable } from '../../actions/welcome_actions'

// Images
import { WelcomeShieldsImage } from '../../components/images'

interface Props {
  index: number
  currentScreen: number
}

// Hack inline style for the Checkbox or Toggle label text.
const hackStyleDiv = {
  color: '#FFFFFF',
  fontFamily: 'Muli,sans-serif',
  fontSize: 22,
  textAlign: 'center' as 'center',
  WebkitFontSmoothing: 'antialiased',
}

export function ShieldsBox(props: Props) {
  const [isP3AEnabled, setIsP3AEnabled] = React.useState<boolean>(false)
  const dispatch = useDispatch()
  const text = getLocale('p3aDesc').split('$1')
  const opt_in = loadTimeData.getBoolean('showP3AOptIn')

  const { index, currentScreen } = props
  const handleP3AToggleChange = (key: string, selected: boolean) => {
    setIsP3AEnabled(selected)
    dispatch(setP3AEnable(selected))
  }

  return (
    <Content
      zIndex={index}
      active={index === currentScreen}
      screenPosition={'1' + (index + 1) + '0%'}
      isPrevious={index <= currentScreen}
    >
      <WelcomeShieldsImage />
      <Title>{getLocale('privacyTitle')}</Title>
      <Paragraph>
        { getLocale('shieldsDesc') }
      </Paragraph>
      {opt_in && (
        <Checkbox
          value={{ 'p3a': isP3AEnabled }}
          onChange={ handleP3AToggleChange }
          >
          <div
            data-key='p3a'
          ><span style={hackStyleDiv}>
            { getLocale('p3aCheckbox') }
          </span></div>
        </Checkbox>
      )}
      <Paragraph>
        {text[0]}
        <a
          href='https://brave.com/p3a'
          target='_blank'
          rel='noopener noreferrer'
        >
          {text[1]}
        </a>
        {text[2]}
        <a
          href='brave://settings/privacy'
          target='_blank'
          rel='noopener noreferrer'
        >
          {text[3]}
        </a>
        {text[4]}
      </Paragraph>
    </Content>
  )
}
