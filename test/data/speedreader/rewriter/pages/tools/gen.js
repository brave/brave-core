const path = require('path')
const fs = require('fs')


const reportDir = path.join(__dirname, '../report/')

// only want to take "url" directories:
var dirs = fs.readdirSync(reportDir, {withFileTypes:true}).filter(d => d.isDirectory() && d.name.includes('.')).map(d => d.name)

console.log(dirs)

fs.writeFileSync(path.join(reportDir, 'reports.json'), JSON.stringify({reports: dirs}, null, 2))