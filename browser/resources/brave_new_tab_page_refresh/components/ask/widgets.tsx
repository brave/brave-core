/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { style } from './widgets.style'
import imgProtonLogo from './ford.svg'
import imgNewsDigest from './news.png'
import imgProton from './backgroundsmall.gif'
import imgTaskIcon from './task-icon.svg'
import imgPlayCircle from './play-circle-icon.svg'
import imgChatIcon from './chat-icon.svg'
import imgFunIcon from './fun-icon.svg'
import imgGame20Q from './game-20q.png'
import imgGameCrossword from './game-crossword.png'
import imgGameHoroscope from './game-horoscope.png'

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

  return (
    <div className='widget weather'>
      <div className='weather-header'>
        <div className='weather-location'>San Francisco, CA</div>
      </div>
      <div className='weather-content'>
        <div className='weather-temp'>68° F</div>
        <div className='weather-description'>
        Steady temperatures expected over the next 4 days. Currently partly cloudy.
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

function UpcomingTasksWidget() {
  return (
    <div className='widget upcoming-tasks'>
      <div className='upcoming-tasks-header'>
        <img 
          alt="" 
          className='upcoming-tasks-icon'
          src={imgTaskIcon}
        />
        <div className='upcoming-tasks-title'>Upcoming tasks</div>
      </div>
      <div className='upcoming-tasks-content'>
        <div className='task-item'>
          <div className='task-description'>Important notifications</div>
          <div className='task-time-badge'>08:15 am</div>
          <button className='task-play-button'>
            <img 
              alt="Play" 
              className='task-play-icon'
              src={imgPlayCircle}
            />
          </button>
        </div>
        <div className='task-item'>
          <div className='task-description'>Daily news</div>
          <div className='task-time-badge'>09:30 am</div>
          <button className='task-play-button'>
            <img 
              alt="Play" 
              className='task-play-icon'
              src={imgPlayCircle}
            />
          </button>
        </div>
        <div className='task-item'>
          <div className='task-description'>Stocks digest</div>
          <div className='task-time-badge'>09:35 am</div>
          <button className='task-play-button'>
            <img 
              alt="Play" 
              className='task-play-icon'
              src={imgPlayCircle}
            />
          </button>
        </div>
        <div className='task-item'>
          <div className='task-description'>Daily news roundup</div>
          <div className='task-time-badge'>09:30 am</div>
          <button className='task-play-button'>
            <img 
              alt="Play" 
              className='task-play-icon'
              src={imgPlayCircle}
            />
          </button>
        </div>
      </div>
    </div>
  )
}

function TaskIdeasWidget() {
  return (
    <div className='widget task-ideas'>
      <div className='task-ideas-header'>
        <img 
          alt="" 
          className='task-ideas-icon'
          src={imgTaskIcon}
        />
        <div className='task-ideas-title'>Task ideas</div>
        <div className='task-ideas-nav-dots'>
          <div className='nav-dot' />
          <div className='nav-dot nav-dot-active' />
          <div className='nav-dot' />
          <div className='nav-dot' />
        </div>
      </div>
      <div className='task-ideas-content'>
        <div className='task-ideas-description'>
          Provide a concise summary of the top 5 news stories from the past 24 hours, focusing on politics, technology, and science. Include one key takeaway for each story. Format as a Custom Newsletter with date, edition, and sources from Grok's web lookups. Deliver daily.
        </div>
        <div className='task-ideas-info'>
          <div className='task-ideas-name'>📰 Daily news roundup</div>
          <div className='task-ideas-schedule'>Daily at 9:00 am</div>
        </div>
      </div>
    </div>
  )
}

function StartChatWidget() {
  return (
    <div className='widget start-chat'>
      <div className='start-chat-header'>
        <img 
          alt="" 
          className='start-chat-icon'
          src={imgChatIcon}
        />
        <div className='start-chat-title'>Start a new chat</div>
      </div>
      <div className='start-chat-content'>
        <button className='chat-prompt-item'>
          What should I wear today?
        </button>
        <button className='chat-prompt-item'>
          What are today's tech headlines?
        </button>
        <button className='chat-prompt-item'>
          What movies are releasing this week?
        </button>
      </div>
    </div>
  )
}

function FunWithLeoWidget() {
  return (
    <div className='widget fun-with-leo'>
      <div className='fun-with-leo-header'>
        <img 
          alt="" 
          className='fun-with-leo-icon'
          src={imgFunIcon}
        />
        <div className='fun-with-leo-title'>Fun with Brave AI</div>
      </div>
      <div className='fun-with-leo-content'>
        <button className='fun-game-item'>
          <img 
            alt="20 questions" 
            className='game-image'
            src={imgGame20Q}
          />
          <div className='game-label'>Play 20 questions</div>
        </button>
        <button className='fun-game-item'>
          <img 
            alt="NYTimes crossword" 
            className='game-image'
            src={imgGameCrossword}
          />
          <div className='game-label'>NYTimes crossword</div>
        </button>
        <button className='fun-game-item'>
          <img 
            alt="Daily Horoscope" 
            className='game-image'
            src={imgGameHoroscope}
          />
          <div className='game-label'>Daily Horoscope</div>
        </button>
      </div>
    </div>
  )
}

type WidgetType = 'privacy-stats' | 'news-digest' | 'upcoming-tasks' | 'proton' | 'world-clock' | 'weather' | 'task-ideas' | 'start-chat' | 'fun-with-leo'

const widgetComponents: Record<WidgetType, React.ComponentType> = {
  'privacy-stats': PrivacyStatsWidget,
  'news-digest': NewsDigestWidget,
  'upcoming-tasks': UpcomingTasksWidget,
  'proton': ProtonWidget,
  'world-clock': WorldClockWidget,
  'weather': WeatherWidget,
  'task-ideas': TaskIdeasWidget,
  'start-chat': StartChatWidget,
  'fun-with-leo': FunWithLeoWidget,
}

export function Widgets() {
  const [widgets, setWidgets] = React.useState<WidgetType[]>([
    'privacy-stats',
    'news-digest',
    'upcoming-tasks',
    'proton',
    'world-clock',
    'weather',
    'task-ideas',
    'start-chat',
    'fun-with-leo',
  ])

  const [draggedIndex, setDraggedIndex] = React.useState<number | null>(null)
  const [dragOverIndex, setDragOverIndex] = React.useState<number | null>(null)

  const handleDragStart = (e: React.DragEvent, index: number) => {
    // Prevent dragging if clicking on interactive elements
    const target = e.target as HTMLElement
    if (target.tagName === 'BUTTON' || target.closest('button')) {
      e.preventDefault()
      return
    }
    
    setDraggedIndex(index)
    if (e.dataTransfer) {
      e.dataTransfer.effectAllowed = 'move'
      e.dataTransfer.setData('text/plain', '') // Required for Firefox
    }
  }

  const handleDragOver = (e: React.DragEvent, index: number) => {
    e.preventDefault()
    e.stopPropagation()
    if (draggedIndex !== null && draggedIndex !== index) {
      setDragOverIndex(index)
    }
  }

  const handleDragLeave = (e: React.DragEvent) => {
    // Only clear drag-over if we're actually leaving the widget container
    const relatedTarget = e.relatedTarget as HTMLElement
    const currentTarget = e.currentTarget as HTMLElement
    if (!currentTarget.contains(relatedTarget)) {
      setDragOverIndex(null)
    }
  }

  const handleDrop = (e: React.DragEvent, dropIndex: number) => {
    e.preventDefault()
    if (draggedIndex === null || draggedIndex === dropIndex) {
      setDraggedIndex(null)
      setDragOverIndex(null)
      return
    }

    const newWidgets = [...widgets]
    // Swap the widgets
    const draggedWidget = newWidgets[draggedIndex]
    const targetWidget = newWidgets[dropIndex]
    newWidgets[draggedIndex] = targetWidget
    newWidgets[dropIndex] = draggedWidget
    
    setWidgets(newWidgets)
    setDraggedIndex(null)
    setDragOverIndex(null)
  }

  const handleDragEnd = () => {
    setDraggedIndex(null)
    setDragOverIndex(null)
  }

  // Split widgets into 3 columns
  const column1 = widgets.slice(0, 3)
  const column2 = widgets.slice(3, 6)
  const column3 = widgets.slice(6, 9)

  const renderWidget = (widgetType: WidgetType, index: number) => {
    const isDragging = draggedIndex === index
    const isDragOver = dragOverIndex === index
    const WidgetComponent = widgetComponents[widgetType]

    return (
      <div
        key={`${widgetType}-${index}`}
        draggable
        onDragStart={(e) => handleDragStart(e, index)}
        onDragOver={(e) => handleDragOver(e, index)}
        onDragLeave={handleDragLeave}
        onDrop={(e) => handleDrop(e, index)}
        onDragEnd={handleDragEnd}
        className={`widget-container ${isDragging ? 'dragging' : ''} ${isDragOver ? 'drag-over' : ''}`}
      >
        <WidgetComponent />
      </div>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      <div className='content-area'>
        {column1.map((widgetType, idx) => renderWidget(widgetType, idx))}
      </div>
      <div className='content-area'>
        {column2.map((widgetType, idx) => renderWidget(widgetType, idx + 3))}
      </div>
      <div className='content-area'>
        {column3.map((widgetType, idx) => renderWidget(widgetType, idx + 6))}
      </div>
    </div>
  )
}

