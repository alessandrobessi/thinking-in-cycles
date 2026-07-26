#!/usr/bin/env python3
"""A minimal, dependency-free flame graph renderer: folded-stack text in,
a static SVG out. This is the "render" step of the capture -> fold ->
render pipeline Chapter 14 requires be kept explicit.

Input format (one line per unique call path, deepest frame last):

    frame1;frame2;frame3 count
    frame1;frame2;frame4 count

This is the same folded-stack format Brendan Gregg's stackcollapse-*.pl
scripts produce, so a .folded file from the real Linux toolchain
(perf script | stackcollapse-perf.pl) renders here unmodified, and a
.folded file from foldstacks.py (macOS `sample`) is equally valid input
on Linux if someone wanted to compare.

Layout convention matches the standard CPU flame graph: the root sits at
the bottom, deeper stack frames stack upward, and each frame's width is
proportional to its share of total samples -- NOT a timeline (never
call it a timeline).

Differential mode (--diff-against=BASELINE.folded) colors each frame by
whether its share of samples grew (red), shrank (blue), or stayed about
the same (grey) relative to the baseline file, for Chapter 15's
before/after comparisons -- the same red/blue convention Brendan Gregg's
differential flame graphs use.

This is a teaching tool, not a replacement for the real FlameGraph
project (https://github.com/brendangregg/FlameGraph): it is static
(no hover/zoom/search) and deliberately small enough to read end to end.
"""
import argparse
import html
import sys

FRAME_HEIGHT = 17
WIDTH = 1200
MIN_TEXT_WIDTH = 28


class Node:
    __slots__ = ("name", "children", "value", "base_value")

    def __init__(self, name):
        self.name = name
        self.children = {}
        self.value = 0
        self.base_value = 0

    def child(self, name):
        if name not in self.children:
            self.children[name] = Node(name)
        return self.children[name]


def read_folded(path):
    stacks = []
    with open(path) as f:
        for line in f:
            line = line.rstrip("\n")
            if not line.strip():
                continue
            frames_part, _, count_part = line.rpartition(" ")
            if not frames_part or not count_part.strip().lstrip("-").isdigit():
                continue
            frames = frames_part.split(";")
            stacks.append((frames, int(count_part)))
    return stacks


def build_tree(stacks, attr="value"):
    root = Node("root")
    for frames, count in stacks:
        node = root
        setattr(node, attr, getattr(node, attr) + count)
        for frame in frames:
            node = node.child(frame)
            setattr(node, attr, getattr(node, attr) + count)
    return root


def merge_baseline(node, baseline_node):
    node.base_value = baseline_node.value if baseline_node else 0
    for name, child in node.children.items():
        merge_baseline(child, baseline_node.children.get(name) if baseline_node else None)


def color_for(node, diff_mode):
    if diff_mode:
        if node.base_value == 0 and node.value > 0:
            return "#d9534f"  # new frame: red
        if node.base_value > 0 and node.value == 0:
            return "#5b9bd5"  # frame disappeared: blue
        delta = node.value - node.base_value
        if abs(delta) <= max(1, node.base_value * 0.05):
            return "#cfcfcf"  # within 5%: grey, treat as unchanged
        return "#d9534f" if delta > 0 else "#5b9bd5"
    # Non-differential: a warm, deterministic palette keyed by name hash
    # ("the hottest color is [not] the hottest function" -- color here is
    # only ever a visual distinguisher between adjacent frames, never a
    # magnitude encoding).
    h = sum(ord(c) for c in node.name) % 5
    return ["#e8985e", "#e6b566", "#eecb8e", "#e3a86a", "#efc48a"][h]


def render(root, total, max_depth, diff_mode):
    height = (max_depth + 2) * FRAME_HEIGHT
    svg = []
    svg.append(
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" '
        f'height="{height}" viewBox="0 0 {WIDTH} {height}" '
        f'font-family="monospace" font-size="11">'
    )
    svg.append(f'<rect width="{WIDTH}" height="{height}" fill="#ffffff"/>')

    def walk(node, depth, x0, width):
        if width <= 0:
            return
        y = height - (depth + 1) * FRAME_HEIGHT
        color = color_for(node, diff_mode) if depth > 0 else "#888888"
        label = node.name if depth > 0 else "root"
        pct = (node.value / total * 100) if total else 0
        svg.append(
            f'<g><title>{html.escape(label)} ({node.value} samples, {pct:.1f}%)</title>'
            f'<rect x="{x0:.2f}" y="{y}" width="{width:.2f}" height="{FRAME_HEIGHT - 1}" '
            f'fill="{color}" stroke="#ffffff" stroke-width="0.5"/>'
        )
        if width >= MIN_TEXT_WIDTH:
            max_chars = max(1, int(width / 6.2))
            text = label if len(label) <= max_chars else label[: max(0, max_chars - 1)] + "…"
            svg.append(
                f'<text x="{x0 + 2:.2f}" y="{y + FRAME_HEIGHT - 5}" fill="#000000">'
                f'{html.escape(text)}</text>'
            )
        svg.append("</g>")

        child_x = x0
        for name in sorted(node.children):
            child = node.children[name]
            child_width = (child.value / node.value * width) if node.value else 0
            walk(child, depth + 1, child_x, child_width)
            child_x += child_width

    walk(root, 0, 0, WIDTH)
    svg.append("</svg>")
    return "\n".join(svg)


def max_depth_of(node, depth=0):
    if not node.children:
        return depth
    return max(max_depth_of(c, depth + 1) for c in node.children.values())


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("folded", help="path to a folded-stack .folded file")
    ap.add_argument("-o", "--output", default="-", help="output SVG path (default: stdout)")
    ap.add_argument("--diff-against", metavar="BASELINE.folded",
                     help="color frames red/blue by change vs. this earlier folded-stack file")
    args = ap.parse_args()

    stacks = read_folded(args.folded)
    if not stacks:
        print(f"no folded stacks found in {args.folded}", file=sys.stderr)
        return 1
    root = build_tree(stacks, attr="value")

    diff_mode = bool(args.diff_against)
    if diff_mode:
        baseline_stacks = read_folded(args.diff_against)
        baseline_root = build_tree(baseline_stacks, attr="value")
        merge_baseline(root, baseline_root)

    total = root.value
    depth = max_depth_of(root)
    svg = render(root, total, depth, diff_mode)

    if args.output == "-":
        sys.stdout.write(svg + "\n")
    else:
        with open(args.output, "w") as f:
            f.write(svg + "\n")
        print(f"wrote {args.output} ({total} total samples, max depth {depth})", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
