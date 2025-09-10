// Example usage of the Weather component with platform auto-detection

import * as React from 'react';
import Weather from './weather';
import { WeatherResult, RichDataProvider, Theme } from './types';

// Mock data for demonstration
const mockWeatherData: WeatherResult = {
  "location": {
    "id": 5392967,
    "name": "Santa Barbara",
    "country": "US",
    "state": "CA",
    "coords": {
      "lat": 34.7333,
      "lon": -120.0343
    },
    "population": 423895,
    "sunrise": 1757511594,
    "sunset": 1757556865,
    "tzoffset": -25200
  },
  "current_time_iso": "2025-09-10T00:19:11",
  "current_weather": {
    "ts": 1757488704,
    "sunrise": 1757511523,
    "sunset": 1757556774,
    "temp": 18.58,
    "feels_like": 18.63,
    "pressure": 1011,
    "humidity": 82,
    "dew_point": 15.45,
    "uvi": 0.0,
    "clouds": 0,
    "visibility": 10000,
    "wind": {
      "speed": 0.89,
      "deg": 326,
      "gust": 3.13
    },
    "weather": {
      "id": 800,
      "main": "clear",
      "description": "clear sky",
      "icon": "01n"
    }
  },
  "daily": [
    {
      "ts": 1757530800,
      "date_i18n": "Wednesday, Sep 10",
      "sunrise": 1757511523,
      "sunset": 1757556774,
      "moonrise": 1757562900,
      "moonset": 1757523420,
      "temperature": {
        "day": 24.33,
        "min": 14.93,
        "max": 26.17,
        "night": 18.14,
        "evening": 24.38,
        "morning": 15.19
      },
      "feels_like": {
        "day": 24.18,
        "night": 17.99,
        "evening": 24.31,
        "morning": 14.54
      },
      "pressure": 1013,
      "humidity": 52,
      "dew_point": 12.28,
      "wind": {
        "speed": 7.6,
        "deg": 252,
        "gust": 7.59
      },
      "clouds": 0,
      "pop": 0.0,
      "uvi": 7.81,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      }
    },
    {
      "ts": 1757617200,
      "date_i18n": "Thursday, Sep 11",
      "sunrise": 1757597965,
      "sunset": 1757643090,
      "moonrise": 1757651580,
      "moonset": 1757614200,
      "temperature": {
        "day": 23.66,
        "min": 16.66,
        "max": 24.09,
        "night": 19.42,
        "evening": 24.09,
        "morning": 16.66
      },
      "feels_like": {
        "day": 23.41,
        "night": 19.14,
        "evening": 23.99,
        "morning": 16.18
      },
      "pressure": 1015,
      "humidity": 51,
      "dew_point": 11.56,
      "wind": {
        "speed": 4.05,
        "deg": 216,
        "gust": 4.35
      },
      "clouds": 0,
      "pop": 0.0,
      "uvi": 8.09,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      }
    },
    {
      "ts": 1757703600,
      "date_i18n": "Friday, Sep 12",
      "sunrise": 1757684406,
      "sunset": 1757729404,
      "moonrise": 1757740620,
      "moonset": 1757705040,
      "temperature": {
        "day": 23.61,
        "min": 18.08,
        "max": 24.11,
        "night": 19.43,
        "evening": 23.34,
        "morning": 18.08
      },
      "feels_like": {
        "day": 23.49,
        "night": 19.15,
        "evening": 23.3,
        "morning": 17.72
      },
      "pressure": 1014,
      "humidity": 56,
      "dew_point": 13.28,
      "wind": {
        "speed": 4.08,
        "deg": 218,
        "gust": 3.74
      },
      "clouds": 0,
      "pop": 0.0,
      "uvi": 7.16,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      }
    },
    {
      "ts": 1757790000,
      "date_i18n": "Saturday, Sep 13",
      "sunrise": 1757770848,
      "sunset": 1757815719,
      "moonrise": 1757830260,
      "moonset": 1757795760,
      "temperature": {
        "day": 24.21,
        "min": 18.67,
        "max": 24.21,
        "night": 20.16,
        "evening": 23.73,
        "morning": 18.67
      },
      "feels_like": {
        "day": 24.17,
        "night": 19.98,
        "evening": 23.75,
        "morning": 18.29
      },
      "pressure": 1014,
      "humidity": 57,
      "dew_point": 13.58,
      "wind": {
        "speed": 3.28,
        "deg": 204,
        "gust": 2.56
      },
      "clouds": 0,
      "pop": 0.0,
      "uvi": 5.54,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      }
    },
    {
      "ts": 1757876400,
      "date_i18n": "Sunday, Sep 14",
      "sunrise": 1757857290,
      "sunset": 1757902033,
      "moonrise": 0,
      "moonset": 1757886120,
      "temperature": {
        "day": 24.34,
        "min": 18.61,
        "max": 26.44,
        "night": 20.25,
        "evening": 26.44,
        "morning": 18.61
      },
      "feels_like": {
        "day": 24.32,
        "night": 19.79,
        "evening": 26.44,
        "morning": 18.33
      },
      "pressure": 1013,
      "humidity": 57,
      "dew_point": 13.93,
      "wind": {
        "speed": 3.21,
        "deg": 222,
        "gust": 4.11
      },
      "clouds": 0,
      "pop": 0.0,
      "uvi": 6.0,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      }
    },
    {
      "ts": 1757962800,
      "date_i18n": "Monday, Sep 15",
      "sunrise": 1757943731,
      "sunset": 1757988348,
      "moonrise": 1757920320,
      "moonset": 1757975940,
      "temperature": {
        "day": 24.67,
        "min": 19.27,
        "max": 25.16,
        "night": 20.08,
        "evening": 23.35,
        "morning": 19.27
      },
      "feels_like": {
        "day": 24.45,
        "night": 19.71,
        "evening": 23.23,
        "morning": 18.71
      },
      "pressure": 1012,
      "humidity": 48,
      "dew_point": 11.32,
      "wind": {
        "speed": 3.78,
        "deg": 214,
        "gust": 3.24
      },
      "clouds": 0,
      "pop": 0.0,
      "uvi": 6.0,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      }
    },
    {
      "ts": 1758049200,
      "date_i18n": "Tuesday, Sep 16",
      "sunrise": 1758030173,
      "sunset": 1758074662,
      "moonrise": 1758010680,
      "moonset": 1758065160,
      "temperature": {
        "day": 24.48,
        "min": 19.42,
        "max": 24.87,
        "night": 20.6,
        "evening": 23.92,
        "morning": 19.42
      },
      "feels_like": {
        "day": 24.34,
        "night": 20.1,
        "evening": 23.91,
        "morning": 18.91
      },
      "pressure": 1013,
      "humidity": 52,
      "dew_point": 12.43,
      "wind": {
        "speed": 3.6,
        "deg": 211,
        "gust": 3.39
      },
      "clouds": 0,
      "pop": 0.0,
      "uvi": 6.0,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      }
    },
    {
      "ts": 1758135600,
      "date_i18n": "Wednesday, Sep 17",
      "sunrise": 1758116614,
      "sunset": 1758160976,
      "moonrise": 1758101160,
      "moonset": 1758153900,
      "temperature": {
        "day": 26.68,
        "min": 20.15,
        "max": 27.24,
        "night": 22.14,
        "evening": 25.96,
        "morning": 20.15
      },
      "feels_like": {
        "day": 26.82,
        "night": 21.66,
        "evening": 25.96,
        "morning": 19.55
      },
      "pressure": 1013,
      "humidity": 44,
      "dew_point": 10.23,
      "wind": {
        "speed": 3.06,
        "deg": 217,
        "gust": 2.71
      },
      "clouds": 0,
      "pop": 0.0,
      "uvi": 6.0,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      }
    }
  ],
  "hours3": [
    {
      "ts": 1757494800,
      "temperature": {
        "temp": 14.21,
        "feels_like": 13.93,
        "min": 13.72,
        "max": 14.21
      },
      "pressure": 1012,
      "weather": {
        "id": 803,
        "main": "clouds",
        "description": "broken clouds",
        "icon": "04n"
      },
      "wind": {
        "speed": 2.05,
        "deg": 345,
        "gust": 2.76
      },
      "humidity": 86,
      "clouds": 79,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757505600,
      "temperature": {
        "temp": 12.72,
        "feels_like": 12.4,
        "min": 11.85,
        "max": 12.72
      },
      "pressure": 1012,
      "weather": {
        "id": 803,
        "main": "clouds",
        "description": "broken clouds",
        "icon": "04n"
      },
      "wind": {
        "speed": 1.87,
        "deg": 4,
        "gust": 2.55
      },
      "humidity": 90,
      "clouds": 69,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757516400,
      "temperature": {
        "temp": 14.24,
        "feels_like": 13.65,
        "min": 14.24,
        "max": 14.24
      },
      "pressure": 1013,
      "weather": {
        "id": 801,
        "main": "clouds",
        "description": "few clouds",
        "icon": "02d"
      },
      "wind": {
        "speed": 1.27,
        "deg": 343,
        "gust": 1.89
      },
      "humidity": 74,
      "clouds": 19,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757527200,
      "temperature": {
        "temp": 19.95,
        "feels_like": 19.18,
        "min": 19.95,
        "max": 19.95
      },
      "pressure": 1013,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 3.36,
        "deg": 275,
        "gust": 2.85
      },
      "humidity": 45,
      "clouds": 10,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757538000,
      "temperature": {
        "temp": 21.89,
        "feels_like": 21.1,
        "min": 21.89,
        "max": 21.89
      },
      "pressure": 1012,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 4.84,
        "deg": 283,
        "gust": 3.67
      },
      "humidity": 37,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757548800,
      "temperature": {
        "temp": 20.35,
        "feels_like": 19.72,
        "min": 20.35,
        "max": 20.35
      },
      "pressure": 1012,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 4.41,
        "deg": 289,
        "gust": 4.07
      },
      "humidity": 49,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757559600,
      "temperature": {
        "temp": 13.23,
        "feels_like": 12.96,
        "min": 13.23,
        "max": 13.23
      },
      "pressure": 1014,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01n"
      },
      "wind": {
        "speed": 1.87,
        "deg": 305,
        "gust": 2.82
      },
      "humidity": 90,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757570400,
      "temperature": {
        "temp": 11.54,
        "feels_like": 11.23,
        "min": 11.54,
        "max": 11.54
      },
      "pressure": 1015,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01n"
      },
      "wind": {
        "speed": 1.3,
        "deg": 7,
        "gust": 1.09
      },
      "humidity": 95,
      "clouds": 3,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757581200,
      "temperature": {
        "temp": 11.27,
        "feels_like": 10.96,
        "min": 11.27,
        "max": 11.27
      },
      "pressure": 1015,
      "weather": {
        "id": 801,
        "main": "clouds",
        "description": "few clouds",
        "icon": "02n"
      },
      "wind": {
        "speed": 1.6,
        "deg": 0,
        "gust": 1.7
      },
      "humidity": 96,
      "clouds": 12,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757592000,
      "temperature": {
        "temp": 10.92,
        "feels_like": 10.42,
        "min": 10.92,
        "max": 10.92
      },
      "pressure": 1015,
      "weather": {
        "id": 801,
        "main": "clouds",
        "description": "few clouds",
        "icon": "02n"
      },
      "wind": {
        "speed": 1.37,
        "deg": 6,
        "gust": 1.47
      },
      "humidity": 90,
      "clouds": 14,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757602800,
      "temperature": {
        "temp": 13.74,
        "feels_like": 13.08,
        "min": 13.74,
        "max": 13.74
      },
      "pressure": 1016,
      "weather": {
        "id": 801,
        "main": "clouds",
        "description": "few clouds",
        "icon": "02d"
      },
      "wind": {
        "speed": 1.11,
        "deg": 319,
        "gust": 1.21
      },
      "humidity": 73,
      "clouds": 13,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757613600,
      "temperature": {
        "temp": 22.6,
        "feels_like": 21.96,
        "min": 22.6,
        "max": 22.6
      },
      "pressure": 1015,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 3.02,
        "deg": 280,
        "gust": 2.63
      },
      "humidity": 40,
      "clouds": 9,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757624400,
      "temperature": {
        "temp": 26.34,
        "feels_like": 26.34,
        "min": 26.34,
        "max": 26.34
      },
      "pressure": 1013,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 4.91,
        "deg": 274,
        "gust": 4.09
      },
      "humidity": 30,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757635200,
      "temperature": {
        "temp": 20.69,
        "feels_like": 20.17,
        "min": 20.69,
        "max": 20.69
      },
      "pressure": 1013,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 4.24,
        "deg": 286,
        "gust": 4.47
      },
      "humidity": 52,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757646000,
      "temperature": {
        "temp": 12.58,
        "feels_like": 12.11,
        "min": 12.58,
        "max": 12.58
      },
      "pressure": 1015,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01n"
      },
      "wind": {
        "speed": 1.61,
        "deg": 314,
        "gust": 1.93
      },
      "humidity": 85,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757656800,
      "temperature": {
        "temp": 12.5,
        "feels_like": 12.05,
        "min": 12.5,
        "max": 12.5
      },
      "pressure": 1015,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01n"
      },
      "wind": {
        "speed": 1.57,
        "deg": 359,
        "gust": 1.32
      },
      "humidity": 86,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757667600,
      "temperature": {
        "temp": 12.88,
        "feels_like": 12.36,
        "min": 12.88,
        "max": 12.88
      },
      "pressure": 1014,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01n"
      },
      "wind": {
        "speed": 1.4,
        "deg": 23,
        "gust": 1.25
      },
      "humidity": 82,
      "clouds": 10,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757678400,
      "temperature": {
        "temp": 13.26,
        "feels_like": 12.63,
        "min": 13.26,
        "max": 13.26
      },
      "pressure": 1014,
      "weather": {
        "id": 801,
        "main": "clouds",
        "description": "few clouds",
        "icon": "02n"
      },
      "wind": {
        "speed": 1.51,
        "deg": 51,
        "gust": 1.34
      },
      "humidity": 76,
      "clouds": 12,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757689200,
      "temperature": {
        "temp": 17.18,
        "feels_like": 16.49,
        "min": 17.18,
        "max": 17.18
      },
      "pressure": 1015,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 0.92,
        "deg": 38,
        "gust": 1.41
      },
      "humidity": 59,
      "clouds": 6,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757700000,
      "temperature": {
        "temp": 26.32,
        "feels_like": 26.32,
        "min": 26.32,
        "max": 26.32
      },
      "pressure": 1014,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 2.12,
        "deg": 272,
        "gust": 2.83
      },
      "humidity": 31,
      "clouds": 3,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757710800,
      "temperature": {
        "temp": 28.62,
        "feels_like": 27.42,
        "min": 28.62,
        "max": 28.62
      },
      "pressure": 1012,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 4.43,
        "deg": 269,
        "gust": 4.42
      },
      "humidity": 27,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757721600,
      "temperature": {
        "temp": 23.53,
        "feels_like": 23.01,
        "min": 23.53,
        "max": 23.53
      },
      "pressure": 1012,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 3.83,
        "deg": 272,
        "gust": 3.46
      },
      "humidity": 41,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757732400,
      "temperature": {
        "temp": 14.77,
        "feels_like": 14.31,
        "min": 14.77,
        "max": 14.77
      },
      "pressure": 1014,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01n"
      },
      "wind": {
        "speed": 1.76,
        "deg": 5,
        "gust": 1.51
      },
      "humidity": 77,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757743200,
      "temperature": {
        "temp": 15.35,
        "feels_like": 14.66,
        "min": 15.35,
        "max": 15.35
      },
      "pressure": 1014,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01n"
      },
      "wind": {
        "speed": 1.23,
        "deg": 21,
        "gust": 1.3
      },
      "humidity": 66,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757754000,
      "temperature": {
        "temp": 14.84,
        "feels_like": 14.05,
        "min": 14.84,
        "max": 14.84
      },
      "pressure": 1013,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01n"
      },
      "wind": {
        "speed": 1.3,
        "deg": 50,
        "gust": 1.23
      },
      "humidity": 64,
      "clouds": 9,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757764800,
      "temperature": {
        "temp": 15.0,
        "feels_like": 14.12,
        "min": 15.0,
        "max": 15.0
      },
      "pressure": 1013,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01n"
      },
      "wind": {
        "speed": 1.32,
        "deg": 70,
        "gust": 1.1
      },
      "humidity": 60,
      "clouds": 8,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757775600,
      "temperature": {
        "temp": 19.11,
        "feels_like": 18.28,
        "min": 19.11,
        "max": 19.11
      },
      "pressure": 1014,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 0.54,
        "deg": 117,
        "gust": 0.7
      },
      "humidity": 46,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757786400,
      "temperature": {
        "temp": 27.76,
        "feels_like": 26.8,
        "min": 27.76,
        "max": 27.76
      },
      "pressure": 1013,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 2.55,
        "deg": 241,
        "gust": 2.38
      },
      "humidity": 27,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757797200,
      "temperature": {
        "temp": 29.59,
        "feels_like": 28.1,
        "min": 29.59,
        "max": 29.59
      },
      "pressure": 1011,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 5.09,
        "deg": 247,
        "gust": 3.74
      },
      "humidity": 25,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757808000,
      "temperature": {
        "temp": 24.46,
        "feels_like": 23.98,
        "min": 24.46,
        "max": 24.46
      },
      "pressure": 1012,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 3.56,
        "deg": 281,
        "gust": 2.82
      },
      "humidity": 39,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757818800,
      "temperature": {
        "temp": 14.94,
        "feels_like": 14.42,
        "min": 14.94,
        "max": 14.94
      },
      "pressure": 1013,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01n"
      },
      "wind": {
        "speed": 1.38,
        "deg": 344,
        "gust": 1.3
      },
      "humidity": 74,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757829600,
      "temperature": {
        "temp": 14.05,
        "feels_like": 13.44,
        "min": 14.05,
        "max": 14.05
      },
      "pressure": 1013,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01n"
      },
      "wind": {
        "speed": 1.57,
        "deg": 341,
        "gust": 1.5
      },
      "humidity": 74,
      "clouds": 1,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757840400,
      "temperature": {
        "temp": 14.47,
        "feels_like": 13.64,
        "min": 14.47,
        "max": 14.47
      },
      "pressure": 1012,
      "weather": {
        "id": 801,
        "main": "clouds",
        "description": "few clouds",
        "icon": "02n"
      },
      "wind": {
        "speed": 0.68,
        "deg": 22,
        "gust": 0.7
      },
      "humidity": 64,
      "clouds": 13,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757851200,
      "temperature": {
        "temp": 13.94,
        "feels_like": 13.11,
        "min": 13.94,
        "max": 13.94
      },
      "pressure": 1012,
      "weather": {
        "id": 801,
        "main": "clouds",
        "description": "few clouds",
        "icon": "02n"
      },
      "wind": {
        "speed": 0.63,
        "deg": 74,
        "gust": 0.61
      },
      "humidity": 66,
      "clouds": 14,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757862000,
      "temperature": {
        "temp": 18.06,
        "feels_like": 17.25,
        "min": 18.06,
        "max": 18.06
      },
      "pressure": 1013,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 0.85,
        "deg": 179,
        "gust": 1.04
      },
      "humidity": 51,
      "clouds": 8,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757872800,
      "temperature": {
        "temp": 25.58,
        "feels_like": 24.98,
        "min": 25.58,
        "max": 25.58
      },
      "pressure": 1013,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 2.46,
        "deg": 244,
        "gust": 1.71
      },
      "humidity": 30,
      "clouds": 4,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757883600,
      "temperature": {
        "temp": 28.2,
        "feels_like": 27.03,
        "min": 28.2,
        "max": 28.2
      },
      "pressure": 1011,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 4.16,
        "deg": 274,
        "gust": 2.69
      },
      "humidity": 25,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757894400,
      "temperature": {
        "temp": 23.62,
        "feels_like": 22.93,
        "min": 23.62,
        "max": 23.62
      },
      "pressure": 1011,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01d"
      },
      "wind": {
        "speed": 4.31,
        "deg": 286,
        "gust": 4.14
      },
      "humidity": 34,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757905200,
      "temperature": {
        "temp": 13.39,
        "feels_like": 12.87,
        "min": 13.39,
        "max": 13.39
      },
      "pressure": 1013,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01n"
      },
      "wind": {
        "speed": 1.68,
        "deg": 327,
        "gust": 1.96
      },
      "humidity": 80,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    },
    {
      "ts": 1757916000,
      "temperature": {
        "temp": 12.42,
        "feels_like": 11.68,
        "min": 12.42,
        "max": 12.42
      },
      "pressure": 1013,
      "weather": {
        "id": 800,
        "main": "clear",
        "description": "clear sky",
        "icon": "01n"
      },
      "wind": {
        "speed": 1.57,
        "deg": 10,
        "gust": 1.38
      },
      "humidity": 75,
      "clouds": 0,
      "pop": 0.0,
      "visibility": 10000
    }
  ],
  "alerts": []
};

const mockProvider: RichDataProvider = {
  name: 'OpenWeatherMap',
  url: 'https://openweathermap.org/',
};

// Example 1: Minimal usage - everything auto-detected
export const MinimalWeatherExample: React.FC = () => {
  return (
    <div>
      <h2>Minimal Usage (Recommended)</h2>
      <Weather data={mockWeatherData} provider={mockProvider} />
    </div>
  );
};

// Example 2: Explicit configuration (overriding defaults)
export const ConfiguredWeatherExample: React.FC = () => {
  return (
    <div>
      <h2>Explicit Configuration</h2>
      <Weather
        data={mockWeatherData}
        provider={mockProvider}
        theme={Theme.dark}         // Force dark theme
        units="us"           // Force Imperial units
        language="en-GB"     // Force British English
        isMobile={false}     // Force desktop layout
      />
    </div>
  );
};

// Example 3: Platform-aware usage with custom handling
export const PlatformAwareExample: React.FC = () => {
  // You can access platform detection utilities directly
  const [userPrefs, setUserPrefs] = React.useState(() => {
    // Only run detection on client-side
    if (typeof window !== 'undefined') {
      return {
        theme: window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light',
        units: navigator.language.includes('US') ? 'us' : 'metric',
        reducedMotion: window.matchMedia('(prefers-reduced-motion: reduce)').matches,
      };
    }
    return { theme: 'light', units: 'metric', reducedMotion: false };
  });

  // Listen for system preference changes
  React.useEffect(() => {
    if (typeof window === 'undefined') return;

    const darkModeQuery = window.matchMedia('(prefers-color-scheme: dark)');
    const motionQuery = window.matchMedia('(prefers-reduced-motion: reduce)');

    const handleDarkModeChange = (e: MediaQueryListEvent) => {
      setUserPrefs(prev => ({ ...prev, theme: e.matches ? 'dark' : 'light' }));
    };

    const handleMotionChange = (e: MediaQueryListEvent) => {
      setUserPrefs(prev => ({ ...prev, reducedMotion: e.matches }));
    };

    darkModeQuery.addEventListener('change', handleDarkModeChange);
    motionQuery.addEventListener('change', handleMotionChange);

    return () => {
      darkModeQuery.removeEventListener('change', handleDarkModeChange);
      motionQuery.removeEventListener('change', handleMotionChange);
    };
  }, []);

  return (
    <div>
      <h2>Platform-Aware with Custom State</h2>
      <p>Current preferences: {JSON.stringify(userPrefs)}</p>
      <Weather
        data={mockWeatherData}
        provider={mockProvider}
        theme={userPrefs.theme as Theme}
        units={userPrefs.units as 'metric' | 'us'}
        // Weather component will still auto-detect language and mobile
      />
    </div>
  );
};

// Example 4: Complete showcase
export const WeatherShowcase: React.FC = () => {
  const [currentExample, setCurrentExample] = React.useState<'minimal' | 'configured' | 'platform'>('minimal');

  return (
    <div style={{ padding: '20px', fontFamily: 'system-ui, sans-serif' }}>
      <h1>React Weather Component Showcase</h1>

      <div style={{ marginBottom: '20px' }}>
        <button
          onClick={() => setCurrentExample('minimal')}
          style={{
            marginRight: '10px',
            backgroundColor: currentExample === 'minimal' ? '#007acc' : '#f0f0f0',
            color: currentExample === 'minimal' ? 'white' : 'black',
            border: 'none',
            padding: '8px 16px',
            borderRadius: '4px',
            cursor: 'pointer'
          }}
        >
          Minimal (Auto-detect)
        </button>
        <button
          onClick={() => setCurrentExample('configured')}
          style={{
            marginRight: '10px',
            backgroundColor: currentExample === 'configured' ? '#007acc' : '#f0f0f0',
            color: currentExample === 'configured' ? 'white' : 'black',
            border: 'none',
            padding: '8px 16px',
            borderRadius: '4px',
            cursor: 'pointer'
          }}
        >
          Configured
        </button>
        <button
          onClick={() => setCurrentExample('platform')}
          style={{
            backgroundColor: currentExample === 'platform' ? '#007acc' : '#f0f0f0',
            color: currentExample === 'platform' ? 'white' : 'black',
            border: 'none',
            padding: '8px 16px',
            borderRadius: '4px',
            cursor: 'pointer'
          }}
        >
          Platform-Aware
        </button>
      </div>

      {currentExample === 'minimal' && <MinimalWeatherExample />}
      {currentExample === 'configured' && <ConfiguredWeatherExample />}
      {currentExample === 'platform' && <PlatformAwareExample />}
    </div>
  );
};

export default WeatherShowcase;
