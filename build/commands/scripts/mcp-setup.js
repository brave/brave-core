#!/usr/bin/env node

const fs = require('fs')
const path = require('path')
const { execSync } = require('child_process')
const chalk = require('chalk')

const isWindows = process.platform === 'win32'
const isMacOS = process.platform === 'darwin'
const isLinux = process.platform === 'linux'

const PROJECT_ROOT = path.resolve(__dirname, '../../../../..')
const BRAVE_CORE_ROOT = path.resolve(__dirname, '../../..')
const CLAUDE_CONFIG_PATH = path.join(
  BRAVE_CORE_ROOT,
  '.claude',
  'mcp_servers.json',
)
const CURSOR_CONFIG_PATH = path.join(
  require('os').homedir(),
  '.cursor',
  'mcp.json',
)
const GLOBAL_CLAUDE_CONFIG_PATH = path.join(
  require('os').homedir(),
  '.claude',
  'mcp_servers.json',
)
const REQUIREMENTS_PATH = path.join(
  BRAVE_CORE_ROOT,
  'agents',
  'mcp',
  'requirements.txt',
)
const VENV_PATH = path.join(BRAVE_CORE_ROOT, 'agents', 'mcp', 'venv')

function log(message, type = 'info') {
  const colors = {
    info: chalk.blue,
    success: chalk.green,
    warning: chalk.yellow,
    error: chalk.red,
  }
  console.log(colors[type](`[MCP Setup] ${message}`))
}

function ensureDirectoryExists(dirPath) {
  if (!fs.existsSync(dirPath)) {
    fs.mkdirSync(dirPath, { recursive: true })
  }
}

function getVenvPythonPath() {
  if (isWindows) {
    return path.join(VENV_PATH, 'Scripts', 'python.exe')
  }
  return path.join(VENV_PATH, 'bin', 'python')
}

function ensureVenv() {
  if (!fs.existsSync(VENV_PATH)) {
    log(`Creating Python virtual environment at: ${VENV_PATH}`)
    try {
      execSync(`python3 -m venv ${VENV_PATH}`, { stdio: 'inherit' })
      log('Virtual environment created successfully.', 'success')
    } catch (error) {
      log(`Failed to create virtual environment: ${error.message}`, 'error')
      log(
        'Please ensure Python 3 and the "venv" module are installed and available.',
        'warning',
      )
      throw error
    }
  }
}

function installMCPDependencies() {
  log('Installing MCP dependencies into virtual environment...')

  const venvPython = getVenvPythonPath()

  try {
    // Install MCP package
    execSync(`"${venvPython}" -m pip install mcp`, { stdio: 'inherit' })

    // Install requirements if they exist
    if (fs.existsSync(REQUIREMENTS_PATH)) {
      execSync(`"${venvPython}" -m pip install -r "${REQUIREMENTS_PATH}"`, {
        stdio: 'inherit',
      })
    }

    log('MCP dependencies installed successfully.', 'success')
  } catch (error) {
    log(`Failed to install dependencies: ${error.message}`, 'error')
    throw error
  }
}

function detectCursorOnWindows() {
  const cursorPaths = [
    path.join(
      require('os').homedir(),
      'AppData',
      'Local',
      'Programs',
      'cursor',
    ),
    'C:\\Users\\'
      + require('os').userInfo().username
      + '\\AppData\\Local\\Programs\\cursor',
  ]
  return cursorPaths.some((p) => fs.existsSync(p))
}

function detectCursorOnMacOS() {
  const cursorPaths = [
    '/Applications/Cursor.app',
    path.join(require('os').homedir(), 'Applications', 'Cursor.app'),
  ]

  if (cursorPaths.some((p) => fs.existsSync(p))) {
    return true
  }

  try {
    execSync('which cursor', { stdio: 'pipe' })
    return true
  } catch (e) {
    return false
  }
}

function detectCursorOnLinux() {
  const cursorPaths = [
    '/usr/bin/cursor',
    '/usr/local/bin/cursor',
    path.join(require('os').homedir(), '.local', 'bin', 'cursor'),
    '/opt/cursor/cursor',
    '/snap/bin/cursor',
  ]

  if (cursorPaths.some((p) => fs.existsSync(p))) {
    return true
  }

  try {
    execSync('which cursor', { stdio: 'pipe' })
    return true
  } catch (e) {
    return false
  }
}

function detectIDEs() {
  const ides = []

  // Check for Claude Code
  const claudeUserDir = path.join(require('os').homedir(), '.claude')
  if (fs.existsSync(claudeUserDir) || process.env.CLAUDE_CODE_API_KEY) {
    ides.push('claude')
  }

  // Check for Cursor
  const cursorUserDir = path.join(require('os').homedir(), '.cursor')
  if (fs.existsSync(cursorUserDir)) {
    ides.push('cursor')
  }

  // Platform-specific cursor detection
  try {
    let cursorFound = false
    if (isWindows) {
      cursorFound = detectCursorOnWindows()
    } else if (isMacOS) {
      cursorFound = detectCursorOnMacOS()
    } else if (isLinux) {
      cursorFound = detectCursorOnLinux()
    }

    if (cursorFound && !ides.includes('cursor')) {
      ides.push('cursor')
    }
  } catch (e) {
    // Ignore detection errors
  }

  return ides
}

function createBaseMCPConfig() {
  const venvPython = getVenvPythonPath()

  return {
    mcpServers: {
      'brave-source': {
        command: venvPython,
        args: ['-m', 'brave.agents.mcp.source_access_server'],
        cwd: PROJECT_ROOT,
        env: {
          BRAVE_CORE_PATH: BRAVE_CORE_ROOT,
          CHROMIUM_SRC_PATH: path.join(PROJECT_ROOT, 'src'),
          PYTHONPATH: path.join(PROJECT_ROOT, 'src'),
          BRAVE_PROJECT_ROOT: PROJECT_ROOT,
        },
        timeout: 30000,
      },
    },
  }
}

function configureClaudeCode(mcpConfig) {
  const braveSourceConfig = mcpConfig.mcpServers['brave-source']

  // Change to brave-core directory for claude mcp add-json command
  const originalCwd = process.cwd()
  process.chdir(BRAVE_CORE_ROOT)

  try {
    // Build the claude mcp add-json command using JSON config
    const configJson = JSON.stringify(braveSourceConfig)

    const command = `claude mcp add-json --scope local brave-source '${configJson}'`

    const output = execSync(command, {
      encoding: 'utf8',
      cwd: BRAVE_CORE_ROOT,
    })
    
    // Check if server already exists in stdout
    if (output && output.includes('already exists')) {
      // Don't log anything, just continue silently
      return
    }
  } catch (error) {
    // Check if server already exists in stderr
    if (error.stdout && error.stdout.includes('already exists')) {
      // Don't log anything, just continue
    } else if (error.stderr && error.stderr.includes('already exists')) {
      // Don't log anything, just continue  
    } else {
      throw error
    }
  }

  log('✅ Configured Claude Code', 'success')
}

function createCursorRules() {
  const cursorRulesDir = path.join(BRAVE_CORE_ROOT, '.cursor', 'rules')
  const cursorRulesPath = path.join(cursorRulesDir, 'brave-development.mdc')
  const bravePromptPath = path.join(
    BRAVE_CORE_ROOT,
    'agents',
    'prompts',
    'brave_developer.md',
  )

  try {
    if (fs.existsSync(bravePromptPath)) {
      const promptContent = fs.readFileSync(bravePromptPath, 'utf8')

      // Create the .cursor/rules directory if it doesn't exist
      ensureDirectoryExists(cursorRulesDir)

      // Format as MDC file with metadata for Project Rules
      const rulesContent = `---
description: Brave Browser Development Assistant
alwaysApply: true
---

${promptContent.replace(/^# Brave Browser Development Assistant\n\n/, '')}`

      fs.writeFileSync(cursorRulesPath, rulesContent)
    }
  } catch (error) {
    log(
      `Warning: Could not create Cursor project rules: ${error.message}`,
      'warning',
    )
  }
}

function configureCursor(mcpConfig) {
  ensureDirectoryExists(path.dirname(CURSOR_CONFIG_PATH))

  // Read existing Cursor config if it exists
  let cursorConfig = { mcpServers: {} }

  if (fs.existsSync(CURSOR_CONFIG_PATH)) {
    try {
      const existing = fs.readFileSync(CURSOR_CONFIG_PATH, 'utf8')
      cursorConfig = JSON.parse(existing)
    } catch (error) {
      log(
        `Warning: Could not parse existing Cursor config: ${error.message}`,
        'warning',
      )
    }
  }

  // Merge configurations
  cursorConfig.mcpServers = {
    ...cursorConfig.mcpServers,
    ...mcpConfig.mcpServers,
  }

  fs.writeFileSync(CURSOR_CONFIG_PATH, JSON.stringify(cursorConfig, null, 2))
  log(`✅ Configured Cursor: ${CURSOR_CONFIG_PATH}`, 'success')

  // Create cursor rules file for automatic Brave development prompt context
  createCursorRules()
}

function createMCPConfig() {
  const mcpConfig = createBaseMCPConfig()

  // Detect which IDEs are available
  const detectedIDEs = detectIDEs()

  if (detectedIDEs.length === 0) {
    detectedIDEs.push('claude', 'cursor')
  }

  // Configure for Claude Code
  if (detectedIDEs.includes('claude')) {
    configureClaudeCode(mcpConfig)
  }

  // Configure for Cursor
  if (detectedIDEs.includes('cursor')) {
    configureCursor(mcpConfig)
  }

  return mcpConfig
}

function updateGlobalConfig(projectConfig) {
  if (process.argv.includes('--global')) {
    log('Updating global Claude configuration...')

    ensureDirectoryExists(path.dirname(GLOBAL_CLAUDE_CONFIG_PATH))

    let globalConfig = { mcpServers: {} }

    // Read existing global config if it exists
    if (fs.existsSync(GLOBAL_CLAUDE_CONFIG_PATH)) {
      try {
        const existing = fs.readFileSync(GLOBAL_CLAUDE_CONFIG_PATH, 'utf8')
        globalConfig = JSON.parse(existing)
      } catch (error) {
        log(
          `Warning: Could not parse existing global config: ${error.message}`,
          'warning',
        )
      }
    }

    // Merge with project config
    globalConfig.mcpServers = {
      ...globalConfig.mcpServers,
      ...projectConfig.mcpServers,
    }

    fs.writeFileSync(
      GLOBAL_CLAUDE_CONFIG_PATH,
      JSON.stringify(globalConfig, null, 2),
    )
    log(`Updated global Claude config: ${GLOBAL_CLAUDE_CONFIG_PATH}`, 'success')
  }
}

function showUsage() {
  console.log(`
${chalk.bold('Brave MCP Server Setup')}

Usage: npm run mcp:setup [options]

${chalk.bold('Options:')}
  --global      Also install to global Claude configuration
  --help        Show this help message
`)
}

async function main() {
  if (process.argv.includes('--help')) {
    showUsage()
    return
  }

  try {
    log(`Setting up MCP server on ${process.platform}...`)

    // Ensure Python virtual environment exists
    ensureVenv()

    // Install Python dependencies
    installMCPDependencies()

    // Create IDE configurations
    const config = createMCPConfig()

    // Update global config if requested
    updateGlobalConfig(config)

    log('✅ MCP setup completed successfully!', 'success')
  } catch (error) {
    log(`Setup failed: ${error.message}`, 'error')
    process.exit(1)
  }
}

if (require.main === module) {
  main()
}
