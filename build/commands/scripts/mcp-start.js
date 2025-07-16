#!/usr/bin/env node

const { spawn } = require('child_process')
const path = require('path')
const fs = require('fs')
const chalk = require('chalk')

const isWindows = process.platform === 'win32'
const PROJECT_ROOT = path.resolve(__dirname, '..', '..', '..', '..', '..')
const VENV_PATH = path.join(
  PROJECT_ROOT,
  'src',
  'brave',
  '.venv_brave_core_mcp',
)

const colors = {
  info: chalk.blue,
  success: chalk.green,
  warning: chalk.yellow,
  error: chalk.red,
}

function log(message, type = 'info') {
  console.log(colors[type](`[MCP Server] ${message}`))
}

function getVenvPythonPath() {
  const pythonExecutable = isWindows
    ? path.join(VENV_PATH, 'Scripts', 'python.exe')
    : path.join(VENV_PATH, 'bin', 'python')

  if (!fs.existsSync(pythonExecutable)) {
    log('Python virtual environment not found.', 'error')
    log(
      `Please run 'npm run mcp:setup' first to create the environment.`,
      'warning',
    )
    return null
  }
  return pythonExecutable
}

function main() {
  try {
    const pythonCmd = getVenvPythonPath()
    if (!pythonCmd) {
      process.exit(1)
    }

    log('Starting Brave source MCP server from virtual environment...')
    log('Press Ctrl+C to stop the server')

    const serverProcess = spawn(
      pythonCmd,
      ['-m', 'brave.agents.mcp.source_access_server'],
      {
        cwd: PROJECT_ROOT,
        stdio: 'inherit',
        env: {
          ...process.env,
          PYTHONPATH: PROJECT_ROOT + '/src',
        },
      },
    )

    process.on('SIGINT', () => {
      log('Stopping MCP server...')
      serverProcess.kill('SIGINT')
      process.exit(0)
    })

    process.on('SIGTERM', () => {
      log('Stopping MCP server...')
      serverProcess.kill('SIGTERM')
      process.exit(0)
    })

    serverProcess.on('error', (error) => {
      log(`Server error: ${error.message}`, 'error')
      process.exit(1)
    })

    serverProcess.on('close', (code) => {
      if (code !== 0) {
        log(`Server exited with code ${code}`, code === 0 ? 'info' : 'error')
      }
      process.exit(code)
    })
  } catch (error) {
    log(`Failed to start server: ${error.message}`, 'error')
    process.exit(1)
  }
}

if (require.main === module) {
  main()
}
