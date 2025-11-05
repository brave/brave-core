/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { style } from './widgets.style'
import imgProtonLogo from './ford.svg'
import imgNewsDigest from './news.png'
import imgProton from './backgroundsmall.gif'

function PrivacyStatsWidget() {
  return (
    <div className='widget privacy-stats'>
      <div className='privacy-stats-header'>
        <div className='privacy-stats-title'>Privacy stats</div>
      </div>
      <div className='privacy-stats-content'>
        <div className='stat-container top'>
          <div className='stat-value'>
            <div className='stat-number'>657,298</div>
          </div>
          <div className='stat-label'>Trackers & ads blocked</div>
        </div>
        <div className='stat-container middle'>
          <div className='stat-value'>
            <div className='stat-number'>1.48</div>
            <div className='stat-unit'>GB</div>
          </div>
          <div className='stat-label'>Bandwidth saved</div>
        </div>
        <div className='stat-container bottom'>
          <div className='stat-value'>
            <div className='stat-number'>24</div>
            <div className='stat-unit'>h</div>
            <div className='stat-number'>24</div>
            <div className='stat-unit'>m</div>
          </div>
          <div className='stat-label'>Time saved</div>
        </div>
      </div>
    </div>
  )
}

function NewsDigestWidget() {
  return (
    <div className='widget news-digest'>
      <div className='news-digest-background'>
        <img 
          alt="" 
          className='news-digest-image' 
          src={imgNewsDigest}
        />
        <div className='news-digest-overlay' />
      </div>
      <div className='news-digest-header'>
        <div className='news-digest-title'>News digest</div>
        <div className='carousel-dot' />
        <div className='carousel-dot' />
        <div className='carousel-dot' />
      </div>
      <div className='news-digest-content'>
        <div className='news-headline'>
          How Google's new Pixel 10 phones compare to one another on paper
        </div>
        <div className='news-footer'>
          <div className='news-logo-container'>
            {/* Logo placeholder - would use actual publisher logo */}
            <svg width="20" height="20" viewBox="0 0 20 20" fill="none" xmlns="http://www.w3.org/2000/svg">
              <foreignObject x="-35" y="-35" width="90" height="90"><div style={{backdropFilter:'blur(17.5px)',clipPath:'url(#bgblur_0_17821_22565_clip_path)',height:'100%',width:'100%'}}></div></foreignObject><g data-figma-bg-blur-radius="35">
              <rect width="20" height="20" rx="5" fill="white" fillOpacity="0.73"/>
              <path d="M6.71875 12.7707L3 4.1665H10.3646V6.42692H6.71875L9.41667 12.7707H6.71875Z" fill="#5200FF"/>
              <path d="M17 4.1665H14.0104L9.05208 16.4165H11.8958L17 4.1665Z" fill="#5200FF"/>
              </g>
              <defs>
              <clipPath id="bgblur_0_17821_22565_clip_path" transform="translate(35 35)"><rect width="20" height="20" rx="5"/>
              </clipPath></defs>
            </svg>
          </div>
          <div className='news-source'>The Verge</div>
        </div>
      </div>
    </div>
  )
}

function WeatherWidget() {
  // Hardcoded weather data for San Francisco
  const weatherData = {
    temperature: 68,
    description: 'Steady temperatures expected over the next 4 days. Currently partly cloudy.'
  }

  return (
    <div className='widget weather'>
      <div className='weather-header'>
        <div className='weather-location'>San Francisco, CA</div>
      </div>
      <div className='weather-content'>
        <div className='weather-temp'>{weatherData.temperature}° F</div>
        <div className='weather-description'>
          {weatherData.description}
        </div>
      </div>
    </div>
  )
}

function ProtonWidget() {
  return (
    <div className='widget proton'>
      <div className='proton-background'>
      <div className='proton-overlay' />
        <img 
          alt=""
          className='proton-image' 
          src={imgProton}
        />
        
      </div>
      <div className='proton-content'>
        <img 
          alt="Proton" 
          className='proton-logo'
          src={imgProtonLogo}
        />
        
      </div>
    </div>
  )
}

function WorldClockWidget() {
  const [currentTime, setCurrentTime] = React.useState(new Date())

  React.useEffect(() => {
    const timer = setInterval(() => {
      setCurrentTime(new Date())
    }, 1000) // Update every second

    return () => clearInterval(timer)
  }, [])

  const formatTime = (date: Date) => {
    const hours = date.getHours()
    const minutes = date.getMinutes()
    const period = hours >= 12 ? 'PM' : 'AM'
    const displayHours = hours % 12 || 12
    const displayMinutes = minutes.toString().padStart(2, '0')
    return {
      time: `${displayHours.toString().padStart(2, '0')}:${displayMinutes}`,
      period
    }
  }

  // Buenos Aires (UTC-3)
  const buenosAires = new Date(currentTime.toLocaleString('en-US', { timeZone: 'America/Argentina/Buenos_Aires' }))
  const buenosAiresFormatted = formatTime(buenosAires)

  // San Francisco (UTC-8/-7 depending on DST)
  const sanFrancisco = new Date(currentTime.toLocaleString('en-US', { timeZone: 'America/Los_Angeles' }))
  const sanFranciscoFormatted = formatTime(sanFrancisco)

  // London (UTC+0/+1 depending on DST)
  const london = new Date(currentTime.toLocaleString('en-US', { timeZone: 'Europe/London' }))
  const londonFormatted = formatTime(london)

  return (
    <div className='widget world-clock'>
      <div className='clock-outer-ring' />
      <div className='clock-inner-ring' />
      <div className='clock-display' />
      <div className='clock-display-gradient' />
      
      <div className='clock-location-label'>San Francisco, CA</div>
      
      <div className='clock-main-time'>
        <div className='clock-shadow clock-main-hours'>88:88</div>
        <div className='clock-time clock-main-hours'>{sanFranciscoFormatted.time}</div>
        <div className='clock-time clock-main-period'>{sanFranciscoFormatted.period}</div>
      </div>
      
      <div className='clock-secondary-time-shadow clock-time-1'>88:88</div>
      <div className='clock-secondary-time clock-time-1'>{buenosAiresFormatted.time}</div>
      <div className='clock-secondary-time clock-time-1 clock-period'>{buenosAiresFormatted.period}</div>
      <div className='clock-location clock-time-1'>Buenos Aires</div>
      
      <div className='clock-secondary-time-shadow clock-time-2'>88:88</div>
      <div className='clock-secondary-time clock-time-2'>{londonFormatted.time}</div>
      <div className='clock-secondary-time clock-time-2 clock-period'>{londonFormatted.period}</div>
      <div className='clock-location clock-time-2'>London</div>
    </div>
  )
}

function EmptyWidget() {
  return (
    <div className='widget empty-widget'>
      <div className='empty-widget-header'>
        <div className='empty-widget-title'>Brave Finance</div>
      </div>
      <div className='empty-widget-content' />
    </div>
  )
}

export function Widgets() {
  return (
    <div data-css-scope={style.scope}>
      <div className='content-area'>
        <PrivacyStatsWidget />
        <NewsDigestWidget />
        <WeatherWidget />
      </div>
      <div className='content-area'>
        <ProtonWidget />
        <WorldClockWidget />
        <EmptyWidget />
      </div>
    </div>
  )
}

