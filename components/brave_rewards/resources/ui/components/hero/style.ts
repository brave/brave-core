import styled from 'styled-components'

interface StyleProps {
  isMobile?: boolean
}

export const StyledHero = styled<StyleProps, 'div'>('div')`
  text-align: center;
  min-height: 610px;
  padding: 60px 0 25px 0;
  border-top-left-radius: ${p => p.isMobile ? 0 : 35}px;
  border-top-right-radius: ${p => p.isMobile ? 0 : 35}px;
  background: linear-gradient(#392DD1, #8C41DE);
  border-bottom-left-radius: 150% 120px;
  border-bottom-right-radius: 150% 120px;

  @media (max-width: 475px) {
    padding-top: 35px;
  }
`
