if (!window.loadTimeData) {
  console.error('window.loadTimeData was not found. Make sure you include strings.m.js or load_time_data.m.js directly. Perhaps there is a timing issue?')
}
export const loadTimeData = window.loadTimeData
