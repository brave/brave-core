import styled from 'styled-components'
import { StyledCard } from '../widgetCard'

import { gradient, color, radius } from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'

export const PromoCard = styled(StyledCard)`
  background: ${gradient.panelBackground};
`
export const PromoButton = styled(Button)`
  background: ${color.material.divider};
  border-radius: ${radius.l};
`
