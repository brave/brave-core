/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import classnames from '$web-common/classnames'
import { getLocale, formatLocale } from '$web-common/locale'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import { useAIChat } from '../../state/ai_chat_context'
import TrulyPrivateIllustration from './truly_private_illustration.svg'
import ChatWithIllustration from './chat_with_illustration.svg'
import BYOMIllustration from './byom_illustration.svg'
import styles from './style.module.scss'

const welcomeCards = [
  {
    title: getLocale(S.CHAT_UI_WELCOME_CARD_TRULY_PRIVATE_TITLE),
    description: getLocale(S.CHAT_UI_WELCOME_CARD_TRULY_PRIVATE_SUBTITLE),
    illustration: TrulyPrivateIllustration,
  },
  {
    title: getLocale(S.CHAT_UI_WELCOME_CARD_TABS_AND_FILES_TITLE),
    description: getLocale(S.CHAT_UI_WELCOME_CARD_TABS_AND_FILES_SUBTITLE),
    illustration: ChatWithIllustration,
  },
  {
    title: getLocale(S.CHAT_UI_WELCOME_BRING_YOUR_OWN_MODEL_TITLE),
    description: getLocale(S.CHAT_UI_WELCOME_BRING_YOUR_OWN_MODEL_SUBTITLE),
    illustration: BYOMIllustration,
  },
]

const subtitle = formatLocale(S.CHAT_UI_WELCOME_GUIDE_SUBTITLE, {
  $1: (content) => <span className={styles.subtitlePurple}>{content}</span>,
})

function WelcomeGuide() {
  const aiChatContext = useAIChat()

  // State
  const [visibleCard, setVisibleCard] = React.useState(0)
  const [userSelectedCard, setUserSelectedCard] = React.useState<
    number | undefined
  >(undefined)

  // Methods
  const handleLearnMoreClicked = React.useCallback(() => {
    const mojomUrl = new Url()
    mojomUrl.url = 'https://brave.com/leo/'

    aiChatContext.uiHandler?.openURL(mojomUrl)
  }, [aiChatContext.uiHandler])

  // Effects
  React.useEffect(() => {
    // Only start auto-cycling if user hasn't selected a card
    if (userSelectedCard !== undefined) {
      return
    }

    const interval = setInterval(() => {
      setVisibleCard((prevIndex) => (prevIndex + 1) % welcomeCards.length)
    }, 10000) // 10 seconds

    return () => clearInterval(interval)
  }, [userSelectedCard])

  return (
    <div className={styles.wrapper}>
      <div className={styles.header}>
        <span className={styles.title}>
          {getLocale(S.CHAT_UI_WELCOME_GUIDE_TITLE)}
        </span>
        <span className={styles.subtitle}>{subtitle}</span>
      </div>
      <div className={styles.cardWrapper}>
        {welcomeCards.map((card, i) => {
          const isSelected =
            userSelectedCard !== undefined
              ? userSelectedCard === i
              : visibleCard === i
          return (
            <div
              className={classnames({
                [styles.card]: true,
                [styles.cardCollapsed]: !isSelected,
              })}
              onClick={() => setUserSelectedCard(i)}
              key={card.title}
            >
              <img
                className={styles.cardGraphic}
                src={card.illustration}
              />
              <div className={styles.cardInfo}>
                <span className={styles.cardTitle}>{card.title}</span>
                <span
                  className={classnames({
                    [styles.cardDescription]: true,
                    [styles.contentHidden]: !isSelected,
                  })}
                >
                  {card.description}
                </span>
              </div>
            </div>
          )
        })}
      </div>
      <Button
        kind='plain-faint'
        size='medium'
        onClick={handleLearnMoreClicked}
      >
        {getLocale(S.CHAT_UI_LEARN_MORE_ABOUT_LEO_AI)}
        <Icon
          slot='icon-after'
          name='launch'
        />
      </Button>
    </div>
  )
}

export default WelcomeGuide
