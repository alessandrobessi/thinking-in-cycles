#!/usr/bin/env python3
"""Converts macOS `sample`(1) call-graph output into folded-stack format:
one line per unique call path, "frame1;frame2;frame3 count" -- the same
textual format Brendan Gregg's stackcollapse-perf.pl produces from
`perf script` output on Linux. This is the "fold" step of the
capture -> fold -> render pipeline Chapter 14 requires be
kept explicit; flamegraph_svg.py in this directory renders the result.

Using the same output format as the standard Linux toolchain means a
folded-stack file produced here (from `sample`) and one produced on
Linux (from `perf script | stackcollapse-perf.pl`) can both be fed to
flamegraph_svg.py unmodified.

`sample`'s call graph is an ASCII tree where each line's indentation
(spaces, with a '+', '!', ':', or '|' character marking branch/
continuation lines) encodes its depth: every level adds exactly two
characters of prefix before the sample count, e.g.:

    1749 Thread_13277929   ...           <- depth 0, prefix "    " (4 chars)
    + 1749 start  (in dyld) + 6076 ...    <- depth 1, prefix "    + " (6 chars)
    +   1749 main  (in cyclelab) ...      <- depth 2, prefix "    +   " (8 chars)

A line is a leaf (and so contributes a folded-stack sample) exactly when
no following line is indented deeper than it -- matching how `sample`
reports inclusive counts at every ancestor but only leaves represent
where a sample actually landed.
"""
import re
import sys

LINE_RE = re.compile(r'^(?P<prefix>[ +!:|]*)(?P<count>\d+)\s+(?P<name>.+)$')


THREAD_ID_RE = re.compile(r'^Thread_\d+$')


def frame_name(raw):
    """'compute_worker  (in cyclelab) + 396,400  [0x...]  compute.c:88'
    -> 'compute_worker'. Folded-stack format uses ';' as a separator, so
    strip any literal ';' out of frame names to keep the format valid.

    Thread root labels ('Thread_13277929') carry a unique ID that
    differs every run, which would make two profiles of the exact same
    call structure look completely different under name-based
    comparison -- normalize them to a bare 'thread' so Chapter 15's
    differential mode compares call structure, not incidental thread IDs."""
    name = re.split(r'\s{2,}', raw.strip(), maxsplit=1)[0]
    if THREAD_ID_RE.match(name):
        return 'thread'
    return name.replace(';', ':')


def parse_sample_output(lines):
    parsed = []
    in_graph = False
    for line in lines:
        if line.strip() == 'Call graph:':
            in_graph = True
            continue
        if not in_graph:
            continue
        m = LINE_RE.match(line.rstrip('\n'))
        if not m:
            if parsed:
                break  # blank line or footer section: end of call graph
            continue
        prefix = m.group('prefix')
        depth = max(0, (len(prefix) - 4) // 2)
        count = int(m.group('count'))
        name = frame_name(m.group('name'))
        parsed.append((depth, count, name))

    stacks = []
    stack = []  # list of (depth, name)
    for i, (depth, count, name) in enumerate(parsed):
        while stack and stack[-1][0] >= depth:
            stack.pop()
        stack.append((depth, name))
        next_depth = parsed[i + 1][0] if i + 1 < len(parsed) else -1
        if next_depth <= depth:
            stacks.append((tuple(n for _, n in stack), count))
    return stacks


def main():
    if len(sys.argv) != 2:
        print("usage: foldstacks.py <sample-output.txt>", file=sys.stderr)
        return 64
    with open(sys.argv[1]) as f:
        lines = f.readlines()
    stacks = parse_sample_output(lines)
    if not stacks:
        print("no call-graph stacks found in input", file=sys.stderr)
        return 1
    for path, count in stacks:
        print(f"{';'.join(path)} {count}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
