import Button, { ButtonProps } from '$web-components/button'
import * as React from 'react'
import styled, { css } from 'styled-components'
import { getLocale } from '$web-common/locale'
import { Heart, HeartOutline } from './Icons'

interface Props extends Omit<ButtonProps, 'isPrimary'> {
    className?: string
    following: boolean
}

const StyledButton = styled.div<{ following: boolean }>`
    &> button {
        --background: #FFF;
        padding: 5px 14px;
        ${p => p.following && css`
        color: var(--interactive5);
        --inner-border-color: var(--interactive5);
        --outer-border-color: var(--interactive5);
    `}
    }
`

export default function FollowButton (props: Props) {
    const { following, className, ...rest } = props
    return <StyledButton className={className} following={following}>
        <Button {...rest} isPrimary={!following}>
            {following ? <>
                {Heart} {getLocale('braveNewsFollowButtonFollowing')}
            </>
            : <>
                {HeartOutline} {getLocale('braveNewsFollowButtonNotFollowing')}
            </>}
        </Button>
    </StyledButton>
}
