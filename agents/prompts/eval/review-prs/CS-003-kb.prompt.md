Review pull request #1 in the fixture repository with the `review-prs` skill,
using the knowledge base's rules:

/review-prs #1 open auto full kb

The skill will fetch the PR's diff and review it against the rules of
`docs/knowledge_base/` that the change needs. Report every violation you find
and attribute each one to its rule's name.

Do not post anything to real GitHub — this is an offline evaluation. Write the
per-PR results files exactly as the skill specifies; the harness reads them.
