---
name: prs-reviewed
description:
  'Get PRs a user reviewed on brave/brave-core. Shows PR number, title, author,
  state, and links. Triggers on: prs reviewed, reviewed prs, review activity,
  what did I review, review history.'
argument-hint: '<username> <num>d'
---

# PRs Reviewed

Query GitHub for pull requests a user has reviewed on `brave/brave-core` within
a given time window.

---

## When to Use

- **Checking review activity** for a team member
- **Weekly/daily standups** — summarizing what someone reviewed
- **Workload analysis** — understanding review distribution

---

## The Job

### Step 1: Parse Arguments

The skill receives arguments in the format: `<username> <num>d`

- `<username>` — GitHub username (e.g., `netzenbot`)
- `<num>d` — Number of days to look back from now (e.g., `3d` means last 3 days)

Both arguments are required. If missing, ask the user for them.

### Step 2: Query GitHub

Calculate the start date by subtracting `<num>` days from today's date, then use
the GitHub CLI to search for reviewed PRs:

```bash
gh api --paginate "search/issues?q=type:pr+repo:brave/brave-core+reviewed-by:<username>+updated:>%3D<YYYY-MM-DD>&per_page=100" --jq '.items[] | {number, title, user: .user.login, state, pull_request: .pull_request.html_url, updated_at}'
```

Where `<YYYY-MM-DD>` is the computed start date.

**Important:** The `updated:>=` filter is a rough filter. After fetching
results, you must verify each PR was actually reviewed by the user within the
requested window by checking review timestamps:

```bash
gh api "repos/brave/brave-core/pulls/<number>/reviews" --jq '[.[] | select(.user.login == "<username>")] | sort_by(.submitted_at) | last | .submitted_at'
```

Only include PRs where the user's most recent review falls within the requested
time window.

### Step 3: Present Results

Display results in a markdown table:

| PR           | Title            | Author | State  | Reviewed   |
| ------------ | ---------------- | ------ | ------ | ---------- |
| [#1234](url) | Fix crash in ... | author | merged | 2026-02-25 |

- **PR**: Link to the PR using `[#number](html_url)` format
- **Title**: PR title (truncate to ~60 chars if very long)
- **Author**: PR author's GitHub username
- **State**: open, closed, or merged (check `pull_request.merged_at` to
  distinguish merged from closed)
- **Reviewed**: Date of the user's most recent review on that PR

Sort by review date, most recent first.

After the table, show a summary: "**N PRs** reviewed by @username in the last
Md"

### Step 4: Handle Edge Cases

- If no PRs found, report: "No PRs reviewed by @username in the last Nd on
  brave/brave-core"
- If `gh` is not authenticated, inform the user to run `gh auth login`
- If rate-limited, inform the user and suggest a shorter time window
