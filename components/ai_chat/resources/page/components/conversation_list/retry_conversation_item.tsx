import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import classnames from 'classnames'
import styles from './style.module.scss'
import { getLocale } from '$web-common/locale'
import { ConversationTurn } from '../../api/page_handler'

interface Props {
    id: number,
    prevItemUuid: string,
    turn: ConversationTurn,
    isLoading: boolean,
    elementRef: React.RefObject<HTMLDivElement> | null,
    onRetrySubmit: (uuid: string) => void,
}

export const RetryConversationItem = (props: Props) => {
    const {
        id,
        prevItemUuid,
        turn,
        isLoading,
        elementRef,
        onRetrySubmit,
      } = props

      let iconName = 'warning-circle-filled'

      const turnClass = classnames({
        [styles.turn]: true,
        [styles.retryConversationItem]: true,
        [styles.turnAI]: true,
      })

      const avatarStyles = classnames({
        [styles.avatar]: true,
        [styles.avatarAI]: true,
      })

      const retryBtn = classnames({
        [styles.buttonBox]: true
      })

    return (
        <>
        <div key={id} ref={elementRef} className={turnClass}>
            <div className={avatarStyles}>
                <Icon name={iconName}/>
            </div>
            <div className={styles.message}>
                <div className={styles.messageTextBox}>
                    {turn.text}
                    {isLoading && <span className={styles.caret}/>}
                </div>
                <Button kind='filled'  onClick={() =>
                    onRetrySubmit(prevItemUuid)}>
                    <span className={retryBtn}>
                    {getLocale('retryButtonLabel')}
                    </span>
                </Button>             
            </div>
        </div>
        </>
    )
}