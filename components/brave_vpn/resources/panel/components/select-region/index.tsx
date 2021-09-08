import * as React from 'react'
import * as S from './style'
import { Radio } from 'brave-ui'
import { CaratStrongLeftIcon } from 'brave-ui/components/icons'

// TODO(nullhook): Fetch from the service api
const data = [
  { region: 'United States' },
  { region: 'Japan' },
  { region: 'Thailand' },
  { region: 'United Kingdom' },
  { region: 'India' },
  { region: 'Vietnam' },
  { region: 'Hong Kong' },
  { region: 'France' },
  { region: 'Italy' },
  { region: 'Vietnam' },
  { region: 'Hong Kong' },
  { region: 'France' },
  { region: 'Italy' }
]

interface Props {
  onDone: Function
}

function SelectRegion (props: Props) {
  const handleClick = () => {
    props.onDone()
  }

  return (
    <S.Box>
      <S.PanelContent>
        <S.PanelHeader>
          <S.BackButton
            type='button'
            onClick={handleClick}
          >
            <CaratStrongLeftIcon />
          </S.BackButton>
        </S.PanelHeader>
        <S.RegionList>
          <Radio
            value={{ 'Japan': true }}
            size={'small'}
            disabled={false}
          >
            {data.map((entry, i) => (
              <div key={i} data-value={entry.region}>
                <S.RegionLabel>{entry.region}</S.RegionLabel>
              </div>
            ))}
          </Radio>
        </S.RegionList>
      </S.PanelContent>
    </S.Box>
  )
}

export default SelectRegion
