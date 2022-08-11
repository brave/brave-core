import styled, { css } from 'styled-components'

export const Link = styled('a')<{ inactive?: boolean }>`
    text-decoration: none;
    color: var(--interactive02);

    ${p => p.inactive && css`
        color: gray;
        :hover {
            cursor: default;
        }
    `}
`
