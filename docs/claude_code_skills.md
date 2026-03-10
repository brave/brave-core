# Claude Code Skills

Claude Code skills are slash commands that automate common development tasks
in `brave-core`. Skills are defined in `.claude/skills/` and best practices
in `.claude/rules/best-practices/`.

## Available Skills

| Skill | Description |
|-------|-------------|
| `/add-best-practice` | Add a new best practice entry to the docs |
| `/fix-bp-docs` | Audit and fix stale references, duplicates, and formatting in best practices docs |
| `/check-milestones` | Check/fix milestones on PRs and issues |
| `/check-upstream-flake` | Check LUCI Analysis for upstream test flakiness |
| `/clean-branches` | Delete local branches whose PRs are merged (Experimental) |
| `/commit` | Create atomic git commits |
| `/force-push-downstream` | Force-push branch + all downstream branches |
| `/impl-review` | Implement PR review feedback |
| `/make-ci-green` | Re-run failed CI jobs (Experimental) |
| `/pr` | Create a pull request for the current branch |
| `/preflight` | Run all preflight checks (format, build, test) |
| `/rebase-downstream` | Rebase a tree of dependent branches |
| `/review` | Code review (PR or local changes) |
| `/top-crashers` | Query Backtrace for top crash data (Experimental) |
| `/uplift` | Cherry-pick fixes to beta/release branches |

## Environment Variables

Some skills require API keys to be set (e.g., in your `.envrc`):

### `/top-crashers`

| Variable | Description |
|----------|-------------|
| `BACKTRACE_API_KEY` | Backtrace API token with `query:post` capability |
| `BACKTRACE_PROJECT` | Backtrace project name (or pass `--project` argument) |

### `/make-ci-green` (Experimental)

| Variable | Description |
|----------|-------------|
| `JENKINS_BASE_URL` | Jenkins CI server base URL |
| `JENKINS_USER` | Jenkins username |
| `JENKINS_TOKEN` | Jenkins API token (from `$JENKINS_BASE_URL/me/configure`) |
