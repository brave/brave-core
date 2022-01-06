###How to update wpr
1. cd src\
2. rm -rf .\tools\perf\page_sets\data\*.wprgo
3. vpython tools\perf\record_wpr desktop_memory_system_health --browser=system  --story-filter <story-filter>
4. ls ./tools/perf/page_sets/data/*.wprgo | xargs upload_to_google_storage.py --bucket=brave-telemetry
5. ls ./tools/perf/page_sets/data/*.wprgo | xargs -I % cp %.sha1 ./brave/tools/perf/page_sets_data/
6. `git add brave\tools\perf\page_sets_data\`
7. `npm run update_patches` && stage && commit
