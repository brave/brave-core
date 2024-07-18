# Brave performance tests

## Job URLs

* [Window](<https://ci.brave.com/job/brave-browser-test-perf-windows/>)
* [MacOS](<https://ci.brave.com/job/brave-browser-test-perf-macos/>)
* [Android](<https://ci.brave.com/job/brave-browser-test-perf-android/>)

## S3 migration

Perf data is currently moving to AWS S3 `brave-perf-data` bucket.
The data is accessible via <https://brave-perf-data.s3.brave.com/{path}>.
The current structure:
.`/perf-profiles/`: test perf profiles

## S3 manual upload

`shasum -a 1 <filename> > <filename>.sha1`
`aws s3 cp <filename> s3://brave-perf-data/<folder>/<filename> \
  --profile go-translate-dev --acl bucket-owner-full-control`

## GSC manual upload

`upload_to_google_storage.py --bucket=brave-telemetry <file>`
the script will produce `.sha1` automatically.

## How to update wpr

* Get write access to `brave-telemetry` GCS bucket (ask @devops-team);
* `cd src/`;
* Remove old downloaded wprs: `rm -rf ./brave/tools/perf/page_sets/data/*.wprgo`;
* Record new wprs: `vpython3 tools/perf/record_wpr <benchmark_name> --browser=system  --story-filter <story-filter>`;
* Upload the archives to the cloud storage: `ls ./brave/tools/perf/page_sets/data/*.wprgo | xargs <upload_cmd>`.
  `*.sha1` files will be generated;
* `cd brave`;
* Review and commit new `.sha1` files (not `.wprgo`) plus new entries in
  `./brave/tools/perf/page_sets_data/*.json`.

## Updating profiles

[Instruction](./updating_test_profiles.md)
