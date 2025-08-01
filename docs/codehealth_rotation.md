# Code Health

## Overview

This is an outline for the CodeHealth rotation, which aims to proactively manage
technical debt and maintain our codebase by keeping it up-to-date with upstream
Chromium. These tasks are essential for keeping the codebase current, clean, and
maintainable.

Additionally, CodeHealth work is a great opportunity to familiarise oneself with
different areas of the codebase, modernising the code, introducing new language
features, and applying the style guide's best practices.

## Tasks

All CodeHealth tasks can be found in this [Slack list linked here](https://bravesoftware.slack.com/lists/T04PX1BU8/F0919BSQXB8).
Most of these tasks are simple enough, and they usually cover things that are
detected at random as needing improvements. If you see something that may be
eligible to the rotation, add it to the backlog, which helps to keep a record of
things that may need improving. In a lot of cases, these items will be mistakes
that go past code review but have since been identified, changes in Chromium
that need to be reflected in Brave, or even updating the codebase to use modern
language features.

> [!IMPORTANT]
> CodeHealth rotations are not a replacement for team driven refactoring. Teams
> are still expected and incentivised to diligently keep their components well
> maintained, and CodeHealth rotations should not be used to shirk away from
> that expectation.

## Rotation Process

To ensure steady, incremental improvements, a weekly rotation is in place.

  * **Staff**: Each `brave-core` pod provides a different volunteer to take up
    on a CodeHealth task each week. Only `brave-core` engineers are expected to
    participate.
  * **Cycles**: The expected time commitment is up to 4 hours per volunteer
    (per week).
  * **Scope**: Work should be kept small enough to be manageable and to not
    disrupt overall team deadlines.
  * **Consistency**: Teams are expected to participate each week to ensure a
    continuous flow of improvements.

## Core Principle: Strictly Better State

It is reasonable to expect that engineer availability can be fluid, and that
tasks may be discontinued before full completion, therefore **every merged**
**change must leave the codebase in a strictly better state than it was**
**before.** Under this principle, partially completed work is still considered
valuable.

## GitHub Workflow

All Code Health work must be tracked in GitHub, and should attain to the
following details.

1.  **Labeling**: Issues should have the label `code-health` and `dev-concern`
    applied, and all PRs/commit messages must have the label `[CodeHealth]`
    (e.g `[CodeHealth] Remove uses of base::StringPrintf pt.3`).
2.  **Issue Granularity**: Each Pull Request must correspond to a single GitHub
    issue.
3.  **Issue Hierarchy**: For CodeHealth items that require multiple PRs, a
    parent issue should be created to track the overall task. The issues for the
    individual PRs must be children of this parent issue.
4.  **Meta Tracker**: All Code Health issues must be descendants of the primary
    Code Health Meta issue: [brave-browser\#45575](https://github.com/brave/brave-browser/issues/45575).

If you are using `gh-cli`, here's an example of a CodeHealth issue.

```bash
gh issue create --repo brave/brave-browser \
--title "Remove all uses of \`base::StringPrintf\` Pt.2" \
--body "This is a mirror rewrite of https://crbug.com/40241565." \
--label "code-health" \
--label "dev-concern" \
--label "OS/Desktop" \
--label "QA/Test-Plan-Specified" \
--label "QA/No" \
--label "release-notes/exclude" \
--assignee cdesouza-chromium
```

## Communication and Task Selection

  * **Kick-off**: A thread will be started in the `#browser-dev` Slack channel
    each Monday to initiate the weekly rotation.
  * **Task Assignment**: Engineers picking up on the rotation should reply to
    the thread with details the task they plan to work on (and PR link
    eventually).
  * **Task Backlog**: The official backlog of available CodeHealth tasks is a
    [Slack list linked here](https://bravesoftware.slack.com/lists/T04PX1BU8/F0919BSQXB8).
  * **Backlog Contributions**: All are encouraged to add potential tasks to this
    backlog as they identify areas of technical debt suitable for work as part
    of CodeHealth rotations.

## Mechanical Changes & AI assistants

Automated or "mechanical" changes are strongly encouraged with CodeHealth tasks.
When python scripts, `sed`/`awk`, or other shell commands are used to generate
changes, these tools and commands should be documented, preferably in the PR
description/commit message, to raise reviewer confidence.

AI tooling for CodeHealth work is still lacking. We have `#ai-dev-tips` in
Slack. Any generalised tooling that can be submitted to our repository to assist
on CodeHealth work will be welcome.
