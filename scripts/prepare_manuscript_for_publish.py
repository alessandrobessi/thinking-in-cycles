#!/usr/bin/env python3
"""prepare_manuscript_for_publish.py -- assembles and renders the
manuscript via Quarto (see publish/_quarto.yml and BLUEPRINT.md Section
26, "Definition of Done for the Book": "the HTML, PDF, and EPUB builds
succeed"). Responsibilities, in order:

  1. confirm every chapter path listed in publish/_quarto.yml's
     `chapters:` tree exists on disk;
  2. run validate_concept_graph.py, validate_chapter_metadata.py, and
     validate_links.py, and refuse to proceed if any reports an error;
  3. copy any figure present in figures/source/ (other than its own
     README) into figures/generated/ if a same-named file isn't already
     there (a no-op today -- figures/source/ has no editable sources
     yet, see its own README -- but real once it does);
  4. invoke `quarto render` from the Quarto project directory, for
     whichever formats are requested.

Exit code: 0 on success, 1 if any pre-flight check fails or the render
itself fails. `--skip-validation` and `--dry-run` exist for local
iteration; neither is the default.
"""
import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    print("PyYAML is required (pip install pyyaml)", file=sys.stderr)
    sys.exit(1)

REPO_ROOT = Path(__file__).resolve().parent.parent


def collect_chapter_paths(quarto_config: dict):
    """Walks _quarto.yml's book.chapters tree (which mixes bare path
    strings and {part: ..., chapters: [...]} dicts) and returns every
    referenced path string."""
    paths = []

    def walk(node):
        if isinstance(node, str):
            paths.append(node)
        elif isinstance(node, dict):
            for child in node.get("chapters", []):
                walk(child)
        elif isinstance(node, list):
            for child in node:
                walk(child)

    walk(quarto_config.get("book", {}).get("chapters", []))
    return paths


def check_chapters_exist(quarto_dir: Path, paths, errors):
    for rel in paths:
        resolved = (quarto_dir / rel).resolve()
        if not resolved.exists():
            errors.append(f"_quarto.yml references '{rel}', which does not exist ({resolved})")


def run_validators(errors):
    validators = [
        "validate_concept_graph.py",
        "validate_chapter_metadata.py",
        "validate_links.py",
    ]
    for v in validators:
        script = REPO_ROOT / "scripts" / v
        result = subprocess.run([sys.executable, str(script)], cwd=REPO_ROOT)
        if result.returncode != 0:
            errors.append(f"{v} reported errors (exit code {result.returncode})")


def refresh_figures(errors):
    src = REPO_ROOT / "figures" / "source"
    dst = REPO_ROOT / "figures" / "generated"
    if not src.is_dir() or not dst.is_dir():
        errors.append(f"expected both {src} and {dst} to exist")
        return
    copied = []
    for f in src.iterdir():
        if f.name == "README.md" or not f.is_file():
            continue
        target = dst / f.name
        if not target.exists():
            shutil.copy2(f, target)
            copied.append(f.name)
    if copied:
        print(f"Copied {len(copied)} figure(s) from figures/source to figures/generated: {copied}")
    else:
        print("No new figures to copy from figures/source to figures/generated.")


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--quarto-dir", default="publish")
    parser.add_argument(
        "--format", default=None,
        help="pass through to 'quarto render --to <format>' (default: render every format in _quarto.yml)",
    )
    parser.add_argument(
        "--skip-validation", action="store_true",
        help="skip the three manuscript validators (local iteration only)",
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="run all pre-flight checks but do not actually invoke quarto render",
    )
    args = parser.parse_args()

    quarto_dir = (REPO_ROOT / args.quarto_dir).resolve()
    config_path = quarto_dir / "_quarto.yml"
    if not config_path.exists():
        print(f"ERROR: no _quarto.yml found at {config_path}", file=sys.stderr)
        return 1

    quarto_config = yaml.safe_load(config_path.read_text())
    errors = []

    chapter_paths = collect_chapter_paths(quarto_config)
    check_chapters_exist(quarto_dir, chapter_paths, errors)
    print(f"Checked {len(chapter_paths)} chapter path(s) referenced in _quarto.yml.")

    if not args.skip_validation:
        run_validators(errors)
    else:
        print("Skipping validators (--skip-validation).", file=sys.stderr)

    refresh_figures(errors)

    if errors:
        for e in errors:
            print(f"ERROR: {e}", file=sys.stderr)
        print("\nRefusing to render: pre-flight checks failed.", file=sys.stderr)
        return 1

    if args.dry_run:
        print("Pre-flight checks passed. --dry-run set, not invoking quarto render.")
        return 0

    if shutil.which("quarto") is None:
        print("ERROR: 'quarto' not found on PATH.", file=sys.stderr)
        return 1

    cmd = ["quarto", "render"]
    if args.format:
        cmd += ["--to", args.format]
    print(f"Running: {' '.join(cmd)} (cwd={quarto_dir})")
    result = subprocess.run(cmd, cwd=quarto_dir)
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
