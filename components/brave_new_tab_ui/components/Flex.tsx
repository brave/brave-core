import styled from 'styled-components'

interface FlexProps {
  align?: 'start' | 'end' | 'center' | 'flex-end' | 'flex-start' | 'self-start' | 'self-end'
  justify?: 'start' | 'end' | 'center' | 'space-between' | 'space-around' | 'space-evenly' | 'left' | 'right'
  direction?: 'row' | 'column' | 'row-reverse' | 'column-reverse'
  gap?: number | string
}

const Flex = styled('div') <FlexProps>`
  display: flex;
  flex-direction: ${p => p.direction};
  justify-content: ${p => p.justify};
  align-items: ${p => p.align};
  gap: ${p => typeof p.gap === 'number' ? `${p.gap}px` : p.gap};
`

export default Flex
