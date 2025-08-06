# Code Health Rotation

## Overview

This is an outline for the CodeHealth rotation, which aims to proactively manage
technical debt. These tasks are essential for keeping the codebase current with
Chromium, clean, and maintainable.

Additionally, CodeHealth work is a great opportunity to familiarise oneself with
different areas of the codebase, modernising the code, introducing new language
features, and applying the style guide's best practices.

## Tasks

All CodeHealth tasks can be found in the [Browser CodeHealth project](https://github.com/orgs/brave/projects/152).
Most of these tasks are simple enough, and they usually cover things that are
detected at random as needing improvements. If you see something that may be
eligible to this rotation, please don't hesitate to add it to the task backlog
for triage, by adding it to the end of [this list](https://github.com/orgs/brave/projects/152/views/3).

Tasks added to the backlog for triage can be just set with a lightweight text
descritpion, and do not necessarily require a Github ticket until actual work
for it is started.

The items in this project usually fit the following category:
 * Mistakes that somehow managed to get past code review, but have since been
   identified
 * Changes in Chromium that need to be reflected in Brave.
 * Updating the codebase to use modern language features.
 * Improving test coverage.
 * Reducing flakiness.

This of course is not an exhaustive list of what may be considered for the
CodeHealth rotation, but it will be the bulk of the tasks involved.

> [!IMPORTANT]
> CodeHealth rotations are not a replacement for team driven refactorings. Teams
> are still expected and incentivised to diligently keep their components well
> maintained, and CodeHealth rotations should not be used to shirk away from
> that expectation.

## Time Commitment

Each `brave-core` engineer is expected to participate in the CodeHealth rotation
monthly. This involves a commitment of **up to 8 hours per month**, with a
required **minimum of one merged Pull Request**.

Tasks should be kept small enough to be manageable, and not disruptive to
overall team dealines. However, pod leads need to be understanding of team
members that are developers that have these obligations.

> [!IMPORTANT]
> CodeHealth rotation is not to be used to pick up on follow-up work detected
> during one's Pull Request code review.

## Core Principle: Strictly Better State

It is reasonable to expect that engineer availability can be fluid, and that
tasks may be discontinued before full completion, therefore **every merged
change must leave the codebase in a strictly better state than it was before.**
Under this principle, partially completed work is still considered valuable.

## GitHub Workflow

All Code Health work must be tracked in GitHub, and should attain to the
following detais.

1.  **Labeling**: Issues should have the label `code-health` applied, and all
    Pull Requests/commit messages must have the label `[CodeHealth]` (e.g
    `[CodeHealth] Remove uses of base::StringPrintf pt.3`).
2.  **Issue Granularity**: Each Pull Request must have its own dedicated Github
    issue.
3.  **Issue Hierarchy**: For CodeHealth items that require multiple Pull
    Requests, a parent issue should be created to track the overall task. The
    issues for the individual Pull Requests must be children of this parent
    issue.

If you are using `gh-cli`, here's an example of a CodeHealth issue being logged.

```bash
gh issue create --repo brave/brave-browser \
--title "Remove all uses of \`base::StringPrintf\` Pt.2" \
--body "This is a mirror rewrite of https://crbug.com/40241565." \
--label "code-health" \
--label "OS/Desktop" \
--label "QA/Test-Plan-Specified" \
--label "QA/No" \
--label "release-notes/exclude" \
--assignee cdesouza-chromium
```

## Communication and Task Selection

  * **Kick-off**: The `#codehealth-rotation` Slack channel is where sync up can
    be done every month.
  * **Task Backlog**: The official CodeHealth rotation task backlog can be found
    in the [Browser CodeHealth project](https://github.com/orgs/brave/projects/152).
  * **Task Assignment**: Engineers picking up on tasks should sync up fist in
    the `#codehealth-rotation`, and make sure that this is visible in the
    [Browser CodeHealth swimlanes](https://github.com/orgs/brave/projects/152/views/4)
  * **Backlog Contributions**: All are required to add potential tasks to the
    [Browser CodeHealth project](https://github.com/orgs/brave/projects/152), if
    they are good fits for CodeHealth rotations. Please reach out in the slack
    channel `#codehealth-rotation` if you feel like any given task belongs in
    the rotation.

## Mechanical Changes & AI assistants

Automated or "mechanical" changes are strongly encouraged with CodeHealth tasks.
When python scripts, `sed`/`awk`, or other shell commands are used to generate
changes, these tools and commands should be documented, preferrably in the
Pull Request description/commit message, to raise reviewer confidence.

AI tooling for CodeHealth work is still lacking. We have `#ai-dev-tips` in
Slack. Any generalised tooling that can be submitted to our repository to assist
on CodeHealh work will be welcome.

## Consistency and Non-Compliance

Consistent participation in the CodeHealth rotation is fundamental. As such,
fulfilling these duties is considered a standard part of an engineer's
responsibilities. Failure to meet the monthly minimums for this work can and
likely will be a factor during performance reviews.

Managers are accountable for ensuring their direct reports meet the program's
expectations. The consistent participation of their team members is a reflection
of a manager's commitment to codebase quality and team accountability.
Consequently, a manager's own performance evaluation will be affected if their
direct reports do not fulfill their CodeHealth duties.
