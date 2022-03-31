import * as React from 'react'
import { BraveWallet, ChartTimelineObjectType } from '../../../constants/types'

// Styled Components
import {
  StyledWrapper,
  ButtonText,
  StyledButton
} from './style'

export interface Props {
  timelineOptions: ChartTimelineObjectType[]
  selectedTimeline: BraveWallet.AssetPriceTimeframe
  onSubmit: (id: BraveWallet.AssetPriceTimeframe) => void
}

export default class ChartControlBar extends React.PureComponent<Props, {}> {
  onTimeSelect = (id: BraveWallet.AssetPriceTimeframe) => () => {
    this.props.onSubmit(id)
  }

  render () {
    const { timelineOptions, selectedTimeline } = this.props
    return (
      <StyledWrapper>
        {timelineOptions.map((t) =>
          <StyledButton key={t.id} onClick={this.onTimeSelect(t.id)} isSelected={selectedTimeline === t.id}>
            <ButtonText isSelected={selectedTimeline === t.id}>{t.name}</ButtonText>
          </StyledButton>
        )}
      </StyledWrapper>
    )
  }
}
