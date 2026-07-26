#!/usr/bin/env python3
"""prepare_manuscript_for_publish.py -- stages reader-facing copies of
the manuscript into publish/chapters/, then renders them with Quarto.

Each chapter file under book/part-*/chapter-*.md has a front-matter
block (H1 title + bold Part/Concept level/Prerequisites/New concepts
metadata) that is authoring scaffolding, not reader-facing content --
it exists to keep glossary.md/misconceptions.md/concept-graph.yaml in
sync while drafting, not to be read. This script strips that block down
to just the title, keeping the body (every "## " section) unchanged,
and writes the result to publish/chapters/<part-dir>/<NN-slug>.md --
dropping the "chapter-" filename prefix too, since chapter numbering is
already carried by the directory's Part ordering and the file's own
leading number.

Appendices (book/appendices/appendix-*.md) have no such block -- they
are copied through unchanged. index.md, preface.md, and
about-the-author.md are hand-authored directly in publish/ (not
generated from book/) and are left alone.

Responsibilities, in order:
  1. stage chapters + appendices into publish/chapters/ as described
     above (publish/chapters/ is cleared first, so a renamed or deleted
     source file can't leave a stale generated file behind);
  2. confirm every path in publish/_quarto.yml's `chapters:` tree
     resolves to a real file;
  3. run validate_concept_graph.py, validate_chapter_metadata.py, and
     validate_links.py against book/ (the real source), and refuse to
     render if any reports an error;
  4. copy any figure present in figures/source/ (other than its own
     README) into figures/generated/ if a same-named file isn't already
     there (a no-op today -- figures/source/ has no editable sources
     yet -- but real once it does);
  5. invoke `quarto render` from publish/, for whichever formats are
     requested.

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
BOOK_DIR = REPO_ROOT / "book"
STAGE_DIR = REPO_ROOT / "publish" / "chapters"
CHAPTER_FILENAME_RE = re.compile(r"^chapter-(\d+-.*\.md)$")
METADATA_LINE_RE = re.compile(
    r"^\*\*(Part|Concept level|Prerequisites|New concepts):\*\*"
)


def strip_chapter_frontmatter(text: str, path: Path) -> str:
    lines = text.splitlines(keepends=True)
    if not lines or not lines[0].startswith("# "):
        raise ValueError(f"{path}: expected an H1 title on line 1")
    title_line = lines[0]
    # Skip the blank line + bold metadata lines that follow the title,
    # stopping at the first line that isn't one of those two shapes.
    i = 1
    while i < len(lines):
        stripped = lines[i].strip()
        if stripped == "" or METADATA_LINE_RE.match(stripped):
            i += 1
            continue
        break
    body = "".join(lines[i:]).lstrip("\n")
    return f"{title_line}\n{body}"


def stage_chapters(errors) -> int:
    if STAGE_DIR.exists():
        shutil.rmtree(STAGE_DIR)
    STAGE_DIR.mkdir(parents=True)

    count = 0
    for src in sorted(BOOK_DIR.glob("part-*/chapter-*.md")):
        m = CHAPTER_FILENAME_RE.match(src.name)
        if not m:
            errors.append(f"{src}: filename doesn't match 'chapter-NN-slug.md'")
            continue
        dest = STAGE_DIR / src.parent.name / m.group(1)
        dest.parent.mkdir(parents=True, exist_ok=True)
        try:
            dest.write_text(strip_chapter_frontmatter(src.read_text(), src))
        except ValueError as e:
            errors.append(str(e))
            continue
        count += 1

    for src in sorted(BOOK_DIR.glob("appendices/appendix-*.md")):
        dest = STAGE_DIR / "appendices" / src.name
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dest)
        count += 1

    print(f"Staged {count} file(s) into {STAGE_DIR.relative_to(REPO_ROOT)}/")
    return count


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


def mirror_figures_into_publish():
    """Typst sandboxes rendering to the Quarto project directory
    (publish/) and refuses any path that resolves outside it, even via
    a real, existing '../' reference -- so a staged chapter's image
    reference (e.g. "../../figures/generated/x.svg", unchanged from its
    book/part-N/ original) can only resolve if a matching
    figures/generated/ directory also exists inside publish/ itself.
    Mirrors the real figures/generated/ into publish/figures/generated/
    so it does, without editing any chapter's own image path."""
    src = REPO_ROOT / "figures" / "generated"
    dst = REPO_ROOT / "publish" / "figures" / "generated"
    if dst.exists():
        shutil.rmtree(dst)
    if not src.is_dir():
        return
    dst.mkdir(parents=True, exist_ok=True)
    for f in src.iterdir():
        if f.name == "README.md" or not f.is_file():
            continue
        shutil.copy2(f, dst / f.name)


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

    errors = []

    stage_chapters(errors)

    quarto_config = yaml.safe_load(config_path.read_text())
    chapter_paths = collect_chapter_paths(quarto_config)
    check_chapters_exist(quarto_dir, chapter_paths, errors)
    print(f"Checked {len(chapter_paths)} chapter path(s) referenced in _quarto.yml.")

    if not args.skip_validation:
        run_validators(errors)
    else:
        print("Skipping validators (--skip-validation).", file=sys.stderr)

    refresh_figures(errors)
    mirror_figures_into_publish()

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
