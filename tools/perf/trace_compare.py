#!/usr/bin/env python3
from dataclasses import dataclass, field
import os
import re
import glob
import gzip
from typing import Dict, List, Optional, Tuple
import numpy as np
from perfetto.trace_processor import TraceProcessor,TraceProcessorConfig
from perfetto.trace_processor.shell import load_shell
from perfetto.trace_processor.platform import PlatformDelegate

from scipy.stats import ttest_ind
import argparse

_DIFF_NEW = 1000000

@dataclass(frozen=True)
class Slice:
    name: str
    category: str
    mojo_interface_tag: str
    thread_name: str
    process_name: str
    posted_from_function_name: str
    posted_from_file_name: str
    is_top_level: bool
    slice_id: int
    trace_file_idx: int

    # TODO: add trace_filename
    def display_name(self, verbose = False, trace_filename: Optional[str] = None):
        if verbose:
            return self.__str__() + f" [{trace_filename or '??'}: id {self.slice_id}]"
        if self.mojo_interface_tag:
            return f"{self.name} ({self.mojo_interface_tag})"
        if self.posted_from_function_name:
            filename = self.posted_from_file_name.split('/')[-1] if self.posted_from_file_name else '??'
            return f"{self.name} ({filename}:{self.posted_from_function_name})"
        return self.name


def ensure_len(l: List[float], length: int):
    if len(l) < length:
        l.extend([0.0] * (length - len(l)))

@dataclass
class Stats:
    files: List[str] = field(default_factory=list)
    slices: Dict[int, Slice] = field(default_factory=dict)
    slice_thread_durations: Dict[int, List[float]] = field(default_factory=dict)
    thread_total_durations: Dict[Tuple[str, str], List[float]] = field(default_factory=dict)


    def process_file(self, filename: str, trace_processor_url: str):
        trace_idx = len(self.files)
        self.files.append(filename)
        with gzip.open(filename, 'rb') as f:
            tp = TraceProcessor(trace=f, addr=trace_processor_url)
            query = '''
            SELECT slice.name, slice.id, slice.parent_id, category, thread_dur,
                   args_1.display_value as posted_from_function_name,
                   args_2.display_value as posted_from_file_name,
                   args_3.display_value as mojo_interface_tag,
                   thread.name as thread_name,
                   process.name as process_name
            FROM slice
            LEFT JOIN args AS args_1 ON args_1.arg_set_id = slice.arg_set_id AND args_1.key = 'task.posted_from.function_name'
              LEFT JOIN args AS args_2 ON args_2.arg_set_id = slice.arg_set_id AND args_2.key = 'task.posted_from.file_name'
            LEFT JOIN args AS args_3 ON args_3.arg_set_id = slice.arg_set_id AND args_3.key = 'chrome_mojo_event_info.mojo_interface_tag'
            LEFT JOIN thread_track ON slice.track_id = thread_track.id
            LEFT JOIN thread ON thread_track.utid = thread.utid
            LEFT JOIN process ON thread.upid = process.upid
            WHERE slice.thread_dur > 0
            '''
            result = tp.query(query)
            for row in result:
                thread_name = re.sub(r'\d+$', '', row.thread_name)
                thread_dur = row.thread_dur / 1000 / 1000  # Convert to ms
                is_top_level = row.parent_id is None

                processed_filename = re.sub(r'../src/', '', row.posted_from_file_name) if row.posted_from_file_name else None
                slice_hash = hash((row.name, row.mojo_interface_tag, processed_filename, row.posted_from_function_name))


                if slice_hash not in self.slices:
                    self.slices[slice_hash] = Slice(name = row.name,
                              category = row.category,
                              mojo_interface_tag = row.mojo_interface_tag,
                              thread_name = thread_name,
                              process_name = row.process_name,
                              posted_from_function_name = row.posted_from_function_name,
                              posted_from_file_name = row.posted_from_file_name,
                              is_top_level = is_top_level,
                              slice_id = row.id,
                              trace_file_idx = trace_idx)

                durations = self.slice_thread_durations.setdefault(slice_hash, [])

                ensure_len(durations, trace_idx + 1)
                durations[trace_idx] += thread_dur

                if is_top_level:
                    thread_total_duration = self.thread_total_durations.setdefault((thread_name, row.process_name), [])
                    ensure_len(thread_total_duration, trace_idx + 1)
                    thread_total_duration[trace_idx] += thread_dur

    def default_thread_duration(self):
        return [0.0] * len(self.files)

    def get_thread_duration(self, slice_id: int):
        index = self.slices.get(slice_id)
        if index is None:
            return self.default_thread_duration()
        return self.slice_thread_durations.get(slice_id, self.default_thread_duration())

def compare_values(before_data: List[float], after_data: List[float], display_name: str, verbose: bool, args) -> Tuple[Optional[float], str]:
    before_dur = np.mean(before_data)
    after_dur = np.mean(after_data)

    if before_dur > 0 and after_dur > 0:
      _, p_value = ttest_ind(before_data, after_data, equal_var=False)
    else:
      p_value = 0

    rel_diff = (after_dur - before_dur) / before_dur if before_dur > 0 else _DIFF_NEW + after_dur

    if abs(before_dur - after_dur) < args.min_abs_diff or abs(rel_diff) < args.min_rel_diff:
        return None, "" # skip small diff

    if p_value > args.min_p_value:
        return None, "" # skip insignificant differences

    sign = '+' if after_dur > before_dur else ''
    if before_dur == 0:
      diff_msg = "New"
    elif after_dur == 0:
      diff_msg = "Removed"
    else:
      diff_msg = f"{sign}{rel_diff * 100:.1f}%"
    before_std = np.std(before_data)
    after_std = np.std(after_data)
    display_name = display_name
    msg = f"{display_name}: {diff_msg}, " \
          f"p-value={p_value:.3f}, "
    if verbose:
      msg += f" ({np.min(before_data):.1f} - {np.max(before_data):.1f} => " \
             f"{np.min(after_data):.1f} - {np.max(after_data):.1f})"
    else:
      msg += f" ({before_dur:.1f}±{before_std:.1f} => {after_dur:.1f}±{after_std:.1f}ms)"
    return rel_diff, msg

def calc_stats(trace_files: List[str], trace_processor_url: str) -> Stats:
    stats = Stats()

    for trace_file in trace_files:
        print(f"Processing {trace_file}")
        stats.process_file(trace_file, trace_processor_url)

    return stats

def compare_series(before: Stats, after: Stats, args):
    differences: List[Tuple[float, str]] = []

    all_threads = set(before.thread_total_durations.keys()).union(after.thread_total_durations.keys())
    for (thread_name, process_name) in all_threads:
        before_durations = before.thread_total_durations.get((thread_name, process_name), [])
        after_durations = after.thread_total_durations.get((thread_name, process_name), [])
        rel_diff, msg = compare_values(before_durations, after_durations, f'{process_name}-{thread_name}', args)
        if rel_diff is not None:
            differences.append((rel_diff, msg))

    all_slices = set(before.slices.keys()).union(after.slices.keys())
    for slice_id in all_slices:
        slice = before.slices.get(slice_id) or after.slices.get(slice_id)
        assert slice is not None

        before_durations = before.get_thread_duration(slice_id)
        after_durations = after.get_thread_duration(slice_id)

        if args.thread_name and slice.thread_name != args.thread_name:
            continue

        if args.process_name and slice.process_name != args.process_name:
            continue

        rel_diff, msg = compare_values(before_durations, after_durations, slice.display_name(args.slice_verbose), args.slice_verbose, args  )

        if rel_diff is not None:
          differences.append((rel_diff, msg))

    differences.sort(key=lambda x: -x[0])
    return differences


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Compare trace files.')
    parser.add_argument('folder1', type=str, help='First folder containing trace files')
    parser.add_argument('folder2', type=str, help='Second folder containing trace files')
    parser.add_argument('--min-p-value', type=float, default=0.05, help='Minimum p-value')
    parser.add_argument('--min-abs-diff', type=float, default=10, help='Minimum absolute difference')
    parser.add_argument('--min-rel-diff', type=float, default=0.05, help='Minimum relative difference in %')
    parser.add_argument('--thread-name', type=str, help='Filter by thread name')
    parser.add_argument('--process-name', type=str, help='Filter by process name')
    parser.add_argument('--slice-verbose', action='store_true', help='Verbose slice output')
    args = parser.parse_args()

    pattern = '**/*.pb.gz'
    f1 = os.path.join(args.folder1, pattern)
    f2 = os.path.join(args.folder2, pattern)
    trace_files1 = glob.glob(f1, recursive=True)
    trace_files2 = glob.glob(f2, recursive=True)

    if len(trace_files1) == 0:
        print(f"No trace files found in {args.folder1}")
        exit(1)

    if len(trace_files2) == 0:
        print(f"No trace files found in {args.folder2}")
        exit(1)

    print(f"Comparing {len(trace_files1)} traces with {len(trace_files2)} traces")

    # TODO(atuchin): fetch trace_processor_shell
    trace_processor_shell_path = "tools/perf/core/perfetto_binary_roller/bin/trace_processor_shell"

    config = TraceProcessorConfig(bin_path=trace_processor_shell_path)
    trace_processor_url, subprocess = load_shell(config.bin_path,
                                  config.unique_port,
                                  config.verbose,
                                  config.ingest_ftrace_in_raw,
                                  config.enable_dev_features,
                                  PlatformDelegate())

    stats_before = calc_stats(trace_files1, trace_processor_url)
    stats_after = calc_stats(trace_files2, trace_processor_url)
    differences = compare_series(stats_before, stats_after, args)
    subprocess.kill()
    subprocess.wait()

    print("Significant differences between the two runs:")
    for diff, message in differences:
        print(f"{message}")
