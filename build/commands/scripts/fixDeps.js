'use strict'

import { createInterface } from 'node:readline'

import { execSync } from 'node:child_process'
import { existsSync, readFileSync, writeFileSync } from 'node:fs'
import { resolve } from 'node:path'

const GHSA_RE = /GHSA-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}/g
const STATE_FILE = '.fix_deps_state.json'

function printUsage() {
  console.log(`Usage: node fixDeps.js <input> [options]

Identifies and fixes vulnerable dependencies from GitHub Security Advisories.

Arguments:
  <input>                GitHub issue URL, advisory URL, or GHSA ID(s)

Options:
  --complete             Resume from saved state — triggers brave-core CI after upstream merges
  --dry-run              Show identification summary without triggering workflows
  --help                 Show this help message

Input formats:
  GitHub issue URL       https://github.com/brave/brave-core/issues/12345
  Issue shorthand        brave/brave-core#12345
  Advisory URL           https://github.com/advisories/GHSA-xxxx-xxxx-xxxx
  GHSA IDs               GHSA-xxxx-xxxx-xxxx GHSA-yyyy-yyyy-yyyy
                         GHSA-xxxx-xxxx-xxxx,GHSA-yyyy-yyyy-yyyy`)
}

function parseInput(raw) {
  const trimmed = raw.trim()
  const issueUrlMatch = trimmed.match(
    /^https?:\/\/github\.com\/([^/]+\/[^/]+)\/issues\/(\d+)$/,
  )
  if (issueUrlMatch) {
    const issueUrl = trimmed
    const body = execSync(`gh issue view ${issueUrl} --json body -q .body`, {
      encoding: 'utf-8',
      stdio: ['pipe', 'pipe', 'pipe'],
    })
    const ghsaIds = body.match(GHSA_RE) || []
    if (ghsaIds.length === 0) {
      throw new Error(`No GHSA IDs found in issue ${issueUrl}`)
    }
    return { ghsaIds: [...new Set(ghsaIds)], issueUrl }
  }

  const shorthandMatch = trimmed.match(/^([^/]+\/[^/]+)#(\d+)$/)
  if (shorthandMatch) {
    const issueUrl = `https://github.com/${shorthandMatch[1]}/issues/${shorthandMatch[2]}`
    return parseInput(issueUrl)
  }

  const advisoryUrlMatch = trimmed.match(
    /^https?:\/\/github\.com\/advisories\/(GHSA-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4})$/,
  )
  if (advisoryUrlMatch) {
    return { ghsaIds: [advisoryUrlMatch[1]] }
  }

  const ids = trimmed
    .split(/[,\s]+/)
    .filter((s) => /^GHSA-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}$/.test(s))
  if (ids.length > 0) {
    return { ghsaIds: [...new Set(ids)] }
  }

  throw new Error(`Cannot parse input: ${trimmed}`)
}

function runNpmAudit(cwd) {
  try {
    const raw = execSync('npm audit --json', {
      cwd,
      encoding: 'utf-8',
      stdio: ['pipe', 'pipe', 'pipe'],
      timeout: 120_000,
    })
    return JSON.parse(raw)
  } catch (err) {
    // npm audit exits non-zero when vulnerabilities are found — parse stdout
    if (err.stdout) {
      return JSON.parse(err.stdout)
    }
    throw err
  }
}

function auditRepos({ wdpDir, leoDir }) {
  const results = {}
  try {
    results['brave-core'] = runNpmAudit(process.cwd())
  } catch {
    console.warn('Warning: npm audit failed in brave-core')
  }
  if (leoDir && existsSync(resolve(leoDir))) {
    try {
      results['brave/leo'] = runNpmAudit(leoDir)
    } catch {
      console.warn('Warning: npm audit failed in leo')
    }
  }
  if (wdpDir && existsSync(resolve(wdpDir))) {
    try {
      results['brave/web-discovery-project'] = runNpmAudit(wdpDir)
    } catch {
      console.warn('Warning: npm audit failed in web-discovery-project')
    }
  }
  return results
}

function extractVulns(auditData, targetGhsaIds) {
  const vulns = []
  const targetSet = new Set(targetGhsaIds)
  for (const [pkg, info] of Object.entries(auditData?.vulnerabilities || {})) {
    const matchingVia = (info.via || []).filter((entry) => {
      if (typeof entry === 'string') {
        return targetSet.has(entry)
      }
      const url = entry.url || ''
      const match = url.match(/GHSA-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}/)
      return match && targetSet.has(match[0])
    })
    for (const via of matchingVia) {
      const ghsaId =
        typeof via === 'string'
          ? via
          : (via.url.match(/GHSA-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}/) || [
              '',
            ])[0]
      vulns.push({
        package: pkg,
        severity: info.severity,
        fixAvailable: info.fixAvailable,
        ghsaId,
      })
    }
  }
  return vulns
}

function traceDependencyChain(packageName) {
  try {
    const tree = execSync(`npm ls "${packageName}" --all`, {
      encoding: 'utf-8',
      cwd: process.cwd(),
      stdio: ['pipe', 'pipe', 'pipe'],
    })
    const hasLeo = tree.includes('@brave/leo')
    return { hasLeo, tree }
  } catch {
    return { hasLeo: false, tree: '' }
  }
}

function identify(ghsaIds, { wdpDir, leoDir, issueUrl }) {
  const auditResults = auditRepos({ wdpDir, leoDir })
  const wdpPackageJson =
    wdpDir && existsSync(resolve(wdpDir, 'package.json'))
      ? JSON.parse(readFileSync(resolve(wdpDir, 'package.json'), 'utf-8'))
      : null

  const advisories = {}
  const leoGhsaIds = new Set()
  const wdpGhsaIds = new Set()
  const braveCoreDirectGhsaIds = new Set()
  let needsHashUpdate = false

  const braveCoreVulns = extractVulns(auditResults['brave-core'], ghsaIds)
  const leoVulns = extractVulns(auditResults['brave/leo'], ghsaIds)
  const wdpVulns = extractVulns(
    auditResults['brave/web-discovery-project'],
    ghsaIds,
  )

  for (const vuln of leoVulns) {
    leoGhsaIds.add(vuln.ghsaId)
    addToAdvisory(advisories, vuln, 'brave/leo', false, null)
  }

  for (const vuln of wdpVulns) {
    wdpGhsaIds.add(vuln.ghsaId)
    addToAdvisory(advisories, vuln, 'brave/web-discovery-project', false, null)
  }

  for (const vuln of braveCoreVulns) {
    const { hasLeo } = traceDependencyChain(vuln.package)
    const isWdpDep =
      wdpPackageJson
      && (wdpPackageJson.dependencies?.[vuln.package]
        || wdpPackageJson.devDependencies?.[vuln.package])

    let via = null

    if (hasLeo) {
      via = '@brave/leo'
      leoGhsaIds.add(vuln.ghsaId)
      addToAdvisory(advisories, vuln, 'brave-core', true, via)
      addToAdvisory(advisories, vuln, 'brave/leo', false, null)
      needsHashUpdate = true
    } else if (isWdpDep) {
      via = 'web-discovery-project'
      wdpGhsaIds.add(vuln.ghsaId)
      addToAdvisory(advisories, vuln, 'brave-core', true, via)
      addToAdvisory(
        advisories,
        vuln,
        'brave/web-discovery-project',
        false,
        null,
      )
      needsHashUpdate = true
    } else {
      braveCoreDirectGhsaIds.add(vuln.ghsaId)
      addToAdvisory(advisories, vuln, 'brave-core', false, null)
    }
  }

  const repos = {}
  if (leoGhsaIds.size > 0) {
    repos['brave/leo'] = {
      ghsaIds: [...leoGhsaIds],
      baseBranch: 'main',
    }
  }
  if (wdpGhsaIds.size > 0) {
    repos['brave/web-discovery-project'] = {
      ghsaIds: [...wdpGhsaIds],
      baseBranch: 'master',
    }
  }

  return {
    advisories,
    issueUrl: issueUrl || null,
    repos,
    braveCore: {
      directGhsaIds: [...braveCoreDirectGhsaIds],
      needsHashUpdate,
    },
  }
}

function addToAdvisory(advisories, vuln, repo, isTransitive, via) {
  if (!advisories[vuln.ghsaId]) {
    advisories[vuln.ghsaId] = {
      severity: vuln.severity,
      package: vuln.package,
      affectedRepos: {},
    }
  }
  advisories[vuln.ghsaId].affectedRepos[repo] = isTransitive
    ? { isTransitive: true, via }
    : { isTransitive: false }
}

function printSummary(state) {
  console.log('\n=== Security Advisory Identification ===')
  for (const [ghsaId, info] of Object.entries(state.advisories)) {
    console.log(`\n${ghsaId} (${info.severity})`)
    console.log(`  Package: ${info.package}`)
    console.log('  Affected repos:')
    for (const [repo, detail] of Object.entries(info.affectedRepos)) {
      if (detail.isTransitive) {
        console.log(`    ${repo} (transitive via ${detail.via})`)
      } else {
        console.log(`    ${repo} (direct)`)
      }
    }
  }

  console.log('\n=== Fix Plan ===')
  const fixSteps = []
  const repos = state.repos || {}
  if (repos['brave/leo']) {
    fixSteps.push(
      `Trigger socket-fix.yml in brave/leo for: ${repos['brave/leo'].ghsaIds.join(', ')}`,
    )
  }
  if (repos['brave/web-discovery-project']) {
    fixSteps.push(
      `Trigger socket-fix.yml in brave/web-discovery-project for: ${repos['brave/web-discovery-project'].ghsaIds.join(', ')}`,
    )
  }
  if (state.braveCore.directGhsaIds.length > 0) {
    fixSteps.push(
      `Fix in brave-core directly: ${state.braveCore.directGhsaIds.join(', ')}`,
    )
  }
  if (state.braveCore.needsHashUpdate) {
    fixSteps.push(
      'After upstream PRs merge, update brave-core DEPS + package.json',
    )
  }
  if (fixSteps.length === 0) {
    fixSteps.push('No action needed — advisories not found in any repo.')
  }
  fixSteps.forEach((step, i) => console.log(`  ${i + 1}. ${step}`))
}

function triggerSocketFix(repo, ghsaIds, issueUrl) {
  const ids = ghsaIds.join(',')
  let cmd = `gh workflow run socket-fix.yml --repo ${repo} -f ghsa_ids="${ids}"`
  if (repo === 'brave/brave-core') {
    if (!issueUrl) {
      throw new Error('issue_link is required for brave-core socket-fix.yml')
    }
    cmd += ` -f issue_link="${issueUrl}"`
  }
  try {
    execSync(cmd, { stdio: 'inherit' })
  } catch (err) {
    throw new Error(
      `Failed to trigger socket-fix.yml in ${repo}: ${err.message}`,
    )
  }
}

async function pollForPR(repo, ghsaIds, options = {}) {
  const pollInterval = options.pollInterval || 15000
  const timeout = options.timeout || 300000
  const expectedPrefix = `socket-fix/${ghsaIds.join('-')}`
  const start = Date.now()

  while (Date.now() - start < timeout) {
    process.stderr.write('.')
    try {
      const raw = execSync(
        `gh pr list --repo ${repo} --head "socket-fix/*" --state open --json number,url,headRefOid,headRefName`,
        { encoding: 'utf-8', stdio: ['pipe', 'pipe', 'pipe'] },
      )
      const prs = JSON.parse(raw)
      const match = prs.find((pr) => pr.headRefName.startsWith(expectedPrefix))
      if (match) {
        process.stderr.write('\n')
        return {
          prNumber: match.number,
          prUrl: match.url,
          headRefOid: match.headRefOid,
          headRefName: match.headRefName,
        }
      }
    } catch {
      // continue polling
    }
    await new Promise((r) => setTimeout(r, pollInterval))
  }
  throw new Error(
    `Timed out waiting for PR in ${repo} after ${timeout / 1000}s`,
  )
}

function addIssueComment(repo, prNumber, issueUrl) {
  try {
    execSync(
      `gh pr comment ${prNumber} --repo ${repo} --body "related to ${issueUrl}"`,
      { stdio: 'inherit' },
    )
  } catch {
    console.warn(
      `Warning: Failed to add issue comment to PR #${prNumber} in ${repo}`,
    )
  }
}

function waitForConfirmation(prompt) {
  return new Promise((resolve) => {
    const rl = createInterface({ input: process.stdin, output: process.stdout })
    rl.question(prompt, (answer) => {
      rl.close()
      resolve(
        answer.trim().length > 0 && (answer[0] === 'y' || answer[0] === 'Y'),
      )
    })
  })
}

function checkExistingSocketPRs(repo) {
  try {
    const raw = execSync(
      `gh pr list --repo ${repo} --head "socket/fix/*" --state open --json number,title,url`,
      { encoding: 'utf-8', stdio: ['pipe', 'pipe', 'pipe'] },
    )
    const prs = JSON.parse(raw)
    if (prs.length > 0) {
      const refs = prs.map((pr) => `#${pr.number}`).join(', ')
      console.warn(
        `Warning: Existing Socket app PRs found in ${repo}: ${refs}. These may conflict with the socket-fix workflow.`,
      )
    }
    return prs
  } catch {
    return []
  }
}

async function executeFixes(state) {
  const repos = state.repos || {}
  const issueUrl = state.issueUrl
  const prResults = []

  const upstreamRepos = ['brave/leo', 'brave/web-discovery-project']

  for (const repo of upstreamRepos) {
    const entry = repos[repo]
    if (!entry) continue

    console.log(`\n--- ${repo} ---`)
    checkExistingSocketPRs(repo)

    console.log(`Triggering socket-fix.yml for: ${entry.ghsaIds.join(', ')}`)
    triggerSocketFix(repo, entry.ghsaIds, null)

    console.log('Polling for PR')
    const pr = await pollForPR(repo, entry.ghsaIds)
    console.log(`PR created: ${pr.prUrl}`)

    if (issueUrl) {
      addIssueComment(repo, pr.prNumber, issueUrl)
    }

    prResults.push({ repo, pr })
  }

  if (state.braveCore.directGhsaIds.length > 0) {
    const repo = 'brave/brave-core'
    console.log(`\n--- ${repo} ---`)

    await checkExistingSocketPRs(repo)

    console.log(
      `Triggering socket-fix.yml for: ${state.braveCore.directGhsaIds.join(', ')}`,
    )
    triggerSocketFix(repo, state.braveCore.directGhsaIds, issueUrl)

    console.log('Polling for PR')
    const pr = await pollForPR(repo, state.braveCore.directGhsaIds)
    console.log(`PR created: ${pr.prUrl}`)

    prResults.push({ repo, pr })
  }

  if (prResults.length > 0) {
    console.log('\n=== PR Summary ===')
    const maxLen = Math.max(...prResults.map((r) => r.repo.length))
    for (const { repo, pr } of prResults) {
      const padded = repo.padEnd(maxLen)
      console.log(`${padded} → #${pr.prNumber} ${pr.prUrl}`)
    }
    if (state.braveCore.needsHashUpdate) {
      console.log(
        '\nRun `npm fix_deps --complete` after upstream PRs merge to update DEPS/package.json.',
      )
    }
  }
}

async function complete() {
  const statePath = resolve(STATE_FILE)
  if (!existsSync(statePath)) {
    console.error(`State file not found: ${STATE_FILE}`)
    process.exit(1)
  }

  const state = JSON.parse(readFileSync(statePath, 'utf-8'))
  const repos = state.repos || {}
  const hashUpdates = {}

  for (const [repo, entry] of Object.entries(repos)) {
    const expectedPrefix = `socket-fix/${entry.ghsaIds.join('-')}`
    let raw
    try {
      raw = execSync(
        `gh pr list --repo ${repo} --head "socket-fix/*" --state all --json number,state,mergeCommit,headRefName`,
        { encoding: 'utf-8', stdio: ['pipe', 'pipe', 'pipe'] },
      )
    } catch {
      console.error(`Failed to query PRs in ${repo}`)
      process.exit(1)
    }

    const prs = JSON.parse(raw)
    const match = prs.find((pr) => pr.headRefName.startsWith(expectedPrefix))

    if (!match) {
      console.error(
        `No PR found in ${repo} matching branch prefix ${expectedPrefix}`,
      )
      process.exit(1)
    }

    if (match.state !== 'MERGED') {
      console.log(`Waiting for ${repo} PR #${match.number} to be merged`)
      process.exit(5)
    }

    hashUpdates[repo] = match.mergeCommit.oid
    console.log(
      `${repo} PR #${match.number} merged (${hashUpdates[repo].slice(0, 8)})`,
    )
  }

  if (Object.keys(hashUpdates).length === 0 && state.braveCore.directGhsaIds.length === 0) {
    console.log('No hash updates or direct fixes needed.')
    process.exit(0)
  }

  const allGhsaIds = new Set()
  for (const entry of Object.values(repos)) {
    for (const id of entry.ghsaIds) {
      allGhsaIds.add(id)
    }
  }
  for (const id of state.braveCore.directGhsaIds) {
    allGhsaIds.add(id)
  }

  const issueUrl = state.issueUrl
  if (!issueUrl) {
    console.error('Error: No issue URL in state file. The brave-core socket-fix.yml requires an issue link.')
    process.exit(1)
  }

  let cmd = `gh workflow run socket-fix.yml --repo brave/brave-core -f ghsa_ids="${[...allGhsaIds].join(',')}" -f issue_link="${issueUrl}"`
  if (hashUpdates['brave/web-discovery-project']) {
    cmd += ` -f wdp_ref="${hashUpdates['brave/web-discovery-project']}"`
  }
  if (hashUpdates['brave/leo']) {
    cmd += ` -f leo_ref="${hashUpdates['brave/leo']}"`
  }

  const lines = ['This will trigger socket-fix.yml in brave/brave-core with:']
  lines.push(`  ghsa_ids:   ${[...allGhsaIds].join(',')}`)
  lines.push(`  issue_link: ${issueUrl}`)
  if (hashUpdates['brave/web-discovery-project']) {
    lines.push(`  wdp_ref:    ${hashUpdates['brave/web-discovery-project']}`)
  }
  if (hashUpdates['brave/leo']) {
    lines.push(`  leo_ref:    ${hashUpdates['brave/leo']}`)
  }
  lines.push('')
  console.log(lines.join('\n'))

  const confirmed = await waitForConfirmation('Proceed? (y/N) ')
  if (!confirmed) {
    console.log('Aborted.')
    process.exit(0)
  }

  try {
    execSync(cmd, { stdio: 'inherit' })
  } catch (err) {
    throw new Error(`Failed to trigger socket-fix.yml in brave/brave-core: ${err.message}`)
  }

  console.log('Polling for brave-core PR')
  const pr = await pollForPR('brave/brave-core', [...allGhsaIds])
  console.log(`PR created: ${pr.prUrl}`)

  execSync(`rm -f ${statePath}`, { stdio: 'pipe' })
}

async function main() {
  const args = process.argv.slice(2)

  if (args.length === 0 || args.includes('--help')) {
    printUsage()
    process.exit(0)
  }

  if (args.includes('--complete')) {
    await complete()
    return
  }

  const input = args.find((a) => !a.startsWith('--'))
  if (!input) {
    printUsage()
    process.exit(1)
  }
  const parsed = parseInput(input)
  console.log(`GHSA IDs: ${parsed.ghsaIds.join(', ')}`)
  if (parsed.issueUrl) {
    console.log(`Issue URL: ${parsed.issueUrl}`)
  }

  const wdpDir = process.env.WDP_DIR || null
  const leoDir = process.env.LEO_DIR || null

  if (!wdpDir && !leoDir) {
    console.error(
      'Error: Both WDP_DIR and LEO_DIR are unset. Set at least one to identify transitive origins.',
    )
    process.exit(2)
  }

  if (wdpDir && !existsSync(resolve(wdpDir))) {
    console.warn(`Warning: WDP_DIR path does not exist: ${wdpDir}`)
  }
  if (leoDir && !existsSync(resolve(leoDir))) {
    console.warn(`Warning: LEO_DIR path does not exist: ${leoDir}`)
  }

  const state = identify(parsed.ghsaIds, {
    wdpDir,
    leoDir,
    issueUrl: parsed.issueUrl,
  })
  printSummary(state)

  if (args.includes('--dry-run')) {
    console.log('\nDry run mode — no workflows triggered.')
    writeFileSync(STATE_FILE, JSON.stringify(state, null, 2))
    console.log(`State saved to ${STATE_FILE}`)
    process.exit(0)
  }

  writeFileSync(STATE_FILE, JSON.stringify(state, null, 2))
  console.log(`\nState saved to ${STATE_FILE}`)

  const repos = state.repos || {}
  const hasAnyActions =
    Object.keys(repos).length > 0 || state.braveCore.directGhsaIds.length > 0
  if (!hasAnyActions) {
    process.exit(0)
  }

  const lines = ['This will:']
  if (repos['brave/leo']) {
    lines.push(
      `  - Trigger socket-fix.yml in brave/leo (${repos['brave/leo'].ghsaIds.join(', ')})`,
    )
  }
  if (repos['brave/web-discovery-project']) {
    lines.push(
      `  - Trigger socket-fix.yml in brave/web-discovery-project (${repos['brave/web-discovery-project'].ghsaIds.join(', ')})`,
    )
  }
  if (state.braveCore.directGhsaIds.length > 0) {
    lines.push(
      `  - Trigger socket-fix.yml in brave/brave-core (${state.braveCore.directGhsaIds.join(', ')})`,
    )
  }
  lines.push('')
  console.log(lines.join('\n'))

  const confirmed = await waitForConfirmation('Proceed? (y/N) ')
  if (!confirmed) {
    console.log('Aborted.')
    process.exit(0)
  }

  await executeFixes(state)
}

try {
  await main()
} catch (err) {
  console.error(`Error: ${err.message}`)
  process.exit(1)
}
