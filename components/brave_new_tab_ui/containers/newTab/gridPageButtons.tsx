import * as React from 'react'
import { useRef, useEffect } from 'react'
import styled from 'styled-components'

const PageIndicator = <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
    <circle cx="50" cy="50" r="50" />
</svg>

const ListPageButtonContainer = styled('div') <{}>`
  display: flex;
  flex-direction: row;

  --list-page-button-size: 8px;
  gap: var(--list-page-button-size);
`

const StyledButton = styled('button')`
    all: unset;

    cursor: pointer;

    color: var(--brave-palette-white);
    opacity: 0.5;

    :hover {
        opacity: 0.8
    }

    width: var(--list-page-button-size);
    height: var(--list-page-button-size);
`

interface GridPageButtonProps {
    page: number
    pageContainerRef: React.MutableRefObject<HTMLDivElement | undefined>
}

function GridPageButton(props: GridPageButtonProps) {
    const handleClick = () => {
        const element = props
            .pageContainerRef
            .current
            ?.children[props.page];
        if (!element) return
        
        // Ideally we'd use |element.scrollIntoView| here but it applies a
        // vertical scroll even when it's not needed, which triggers the Brave
        // News peek.
        // |element.scrollIntoViewIfNeeded| is also out, because it doesn't
        // support animating the scroll.
        props.pageContainerRef.current?.scrollTo({ left: element['offsetLeft'], behavior: 'smooth' });
    };

    return <StyledButton onClick={handleClick}>
        {PageIndicator}
    </StyledButton>
}

const GridPageIndicatorContainer = styled('div') <{}>`
  position: absolute;
  color: var(--brave-palette-white);

  width: var(--list-page-button-size);
  height: var(--list-page-button-size);
`

export const GridPageButtons = (props: { numPages: number, pageContainerRef: React.MutableRefObject<HTMLDivElement | undefined> }) => {
    const pages = [...Array(props.numPages).keys()]
    const indicatorRef = useRef<HTMLDivElement>()

    useEffect(() => {
        const el = props.pageContainerRef.current
        if (!el) return

        const scrollHandler = () => {
            const percent = 100 * (el.scrollLeft) / (el.scrollWidth - el.clientWidth)

            // This is so we take into account the gaps between the icons:
            // Page two for: * * * * is the 3rd slot
            //                 ^
            //               1234567
            const translationX = percent * (props.numPages - 1) * 2
            indicatorRef.current?.setAttribute('style', `transform: translateX(${translationX}%)`)
        }
        el.addEventListener('scroll', scrollHandler)

        return () => el.removeEventListener('scroll', scrollHandler)
    }, [props.pageContainerRef, props.numPages])

    return <ListPageButtonContainer>
        {pages.map(page => <GridPageButton
            key={page}
            page={page}
            pageContainerRef={props.pageContainerRef} />)}
        <GridPageIndicatorContainer ref={indicatorRef as any}>
            {PageIndicator}
        </GridPageIndicatorContainer>
    </ListPageButtonContainer>
}
