import Button, { ButtonProps } from '$web-components/button'
import * as React from 'react'
import styled, { css } from 'styled-components'
import { getLocale } from '$web-common/locale'
import { Heart, HeartOutline } from './Icons'

interface Props extends Omit<ButtonProps, 'isPrimary'> {
    following: boolean
}

const StyledButton = styled(Button)`
    padding: 5px 14px;
    --background: #fff;
    ${p => !p.isPrimary && css`
        color: var(--interactive5);
        --inner-border-color: var(--interactive5);
        --outer-border-color: var(--interactive5);
    `}
`

export default function FollowButton (props: Props) {
    const { following, ...rest } = props
    return <StyledButton {...rest} isPrimary={!following}>
        {following ? <>
            {Heart} {getLocale('braveNewsFollowButtonFollowing')}
        </>
        : <>
            {HeartOutline} {getLocale('braveNewsFollowButtonNotFollowing')}
        </>}
    </StyledButton>
}
