use std::fs;
use std::num::NonZeroUsize;

pub(crate) fn num_threads() -> Option<NonZeroUsize> {
    num_threads_from_stat().or_else(num_threads_from_status)
}

fn num_threads_from_stat() -> Option<NonZeroUsize> {
    fs::read_to_string("/proc/self/stat")
        .ok()
        .as_ref()
        // Skip past the pid and (process name) fields
        .and_then(|stat| stat.rsplit(')').next())
        // 20th field, less the two we skipped
        .and_then(|rstat| rstat.split_whitespace().nth(17))
        .and_then(|num_threads| num_threads.parse::<usize>().ok())
        .and_then(NonZeroUsize::new)
}

fn num_threads_from_status() -> Option<NonZeroUsize> {
    fs::read_to_string("/proc/self/status")
        .ok()
        .as_ref()
        .and_then(|status| {
            status
                .lines()
                .find(|line| line.starts_with("Threads:"))
                .and_then(|line| line["Threads:".len()..].trim().parse::<usize>().ok())
        })
        .and_then(NonZeroUsize::new)
}
