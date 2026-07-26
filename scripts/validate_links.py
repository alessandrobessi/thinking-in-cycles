#!/usr/bin/env python3
"""validate_links.py -- checks internal links and referenced lab paths
(BLUEPRINT.md Section 21, "Validation Rules"):

  - "internal links": every Markdown relative link `[text](path)` (i.e.
    not http(s):// or mailto:) under book/, templates/, references/, and
    the top-level registries resolves to a real file, relative to the
    linking file's own directory.
  - "referenced lab paths": every repo-relative path mentioned anywhere
    in those same files (inline code spans, fenced code blocks, or plain
    prose) that starts with a known top-level directory
    (labs/, scripts/, book/, references/, templates/, figures/,
    publish/) resolves to a real file or directory, relative to the
    repository root. Tokens containing a wildcard ('*') or looking like
    a placeholder (e.g. 'chNN', 'partN') are skipped, since those are
    intentionally generic prose, not a literal path.

Exit code: 0 if no issue is found, 1 otherwise.
"""
import argparse
import re
import sys
from pathlib import Path

MD_LINK_RE = re.compile(r"\[[^\]]*\]\(([^)]+)\)")
REPO_PATH_RE = re.compile(
    r"\b((?:labs|scripts|book|references|templates|figures|publish)/"
    r"[A-Za-z0-9_./-]+)"
)
SCAN_GLOBS = [
    "book/**/*.md",
    "templates/*.md",
    "references/**/*.md",
    "glossary.md",
    "misconceptions.md",
    "analogy-registry.md",
    "concept-graph.md",
]
PLACEHOLDER_RE = re.compile(r"ch[A-Z0-9]|part[A-Z0-9]|<|>|\{|\}")


def scan_files(root: Path):
    seen = set()
    for pattern in SCAN_GLOBS:
        for path in root.glob(pattern):
            if path.is_file() and path not in seen:
                seen.add(path)
                yield path


def check_markdown_links(path: Path, text: str, errors):
    for target in MD_LINK_RE.findall(text):
        target = target.strip()
        if not target or target.startswith(("http://", "https://", "mailto:")):
            continue
        anchor_stripped = target.split("#", 1)[0]
        if not anchor_stripped:
            continue  # pure same-page anchor, e.g. "#section"
        resolved = (path.parent / anchor_stripped).resolve()
        if not resolved.exists():
            errors.append(f"{path}: broken link to '{target}' (resolved: {resolved})")


def check_repo_paths(root: Path, path: Path, text: str, errors, warnings):
    root = root.resolve()
    for token in set(REPO_PATH_RE.findall(text)):
        clean = token.rstrip(".,;:)`'\"")
        if "*" in clean or PLACEHOLDER_RE.search(Path(clean).name):
            continue
        resolved = (root / clean)
        if not resolved.exists():
            errors.append(f"{path}: referenced path '{clean}' does not exist")


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--root", default=".")
    args = parser.parse_args()

    root = Path(args.root)
    errors, warnings = [], []
    files = list(scan_files(root))
    if not files:
        errors.append(f"no files matched under {root} for the configured scan globs")

    for path in files:
        text = path.read_text()
        check_markdown_links(path, text, errors)
        check_repo_paths(root, path, text, errors, warnings)

    for w in warnings:
        print(f"WARNING: {w}", file=sys.stderr)
    for e in errors:
        print(f"ERROR: {e}", file=sys.stderr)

    print(
        f"\nvalidate_links.py: {len(files)} files checked, "
        f"{len(errors)} error(s), {len(warnings)} warning(s)",
        file=sys.stderr,
    )
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
