import * as React from 'react'
import { ChartTimelineObjectType, ChartTimelineType } from '../../../constants/types'

// Styled Components
import {
  StyledWrapper,
  ButtonText,
  StyledButton,
  Dot
} from './style'

export interface Props {
  timelineOptions: ChartTimelineObjectType[]
  selectedTimeline: ChartTimelineType
  onSubmit: (id: ChartTimelineType) => void
}

export default class ChartControlBar extends React.PureComponent<Props, {}> {

  onTimeSelect = (id: ChartTimelineType) => () => {
    this.props.onSubmit(id)
  }

  render () {
    const { timelineOptions, selectedTimeline } = this.props
    return (
      <StyledWrapper>
        {timelineOptions.map((t) =>
          <StyledButton key={t.id} onClick={this.onTimeSelect(t.id)} isSelected={selectedTimeline === t.id}>
            {t.id === '5MIN' && <Dot isSelected={selectedTimeline === t.id} />}
            <ButtonText isSelected={selectedTimeline === t.id}>{t.name}</ButtonText>
          </StyledButton>
        )}
      </StyledWrapper>
    )
  }
}
