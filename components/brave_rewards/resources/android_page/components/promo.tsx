/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as style from './promo.style'
import { CloseIcon } from '../../shared/components/icons/close_icon'
import { LocaleContext } from '../../shared/lib/locale_context'

interface Props {
  title: string
  link: string
  copy: React.ReactNode
  disclaimer?: React.ReactNode
  onDismissPromo: () => void
}

export function Promo (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  const onDismiss = (evt: React.MouseEvent) => {
    evt.preventDefault()
    props.onDismissPromo()
  }

  function onLearnMore () {
    window.open(props.link, '_blank', 'noopener')
  }

  const {
    copy,
    title,
    disclaimer
  } = props

  return (
    <style.root>
      <style.closeIcon onClick={onDismiss}>
        <CloseIcon />
      </style.closeIcon>
      <style.content>
        <style.title>
          {title}
        </style.title>
        <style.copy>
          {copy}
        </style.copy>
        {
          disclaimer &&
          <style.disclaimer>
            {disclaimer}
          </style.disclaimer>
        }
        <style.learnMoreButton onClick={onLearnMore}>
          {getString('promoLearnMore')}
        </style.learnMoreButton>
        <style.dismissButton onClick={onDismiss}>
          {getString('promoDismiss')}
        </style.dismissButton>
        <style.background className={'bitflyer-verification-promo'} />
      </style.content>
    </style.root>
  )
}
