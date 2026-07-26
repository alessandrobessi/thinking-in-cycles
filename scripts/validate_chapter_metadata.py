#!/usr/bin/env python3
"""validate_chapter_metadata.py -- checks chapter structure and metadata
(BLUEPRINT.md Section 21, "Validation Rules", and Section 25, "Definition
of Done for a Chapter"):

  - chapter numbering and filenames: `chapter-NN-slug.md`'s NN matches the
    file's own "# Chapter NN --" heading.
  - part and chapter order: each `part-N-*` directory holds exactly the
    five contiguous chapters that Part N owns (BLUEPRINT.md's fixed
    5-chapters-per-Part structure), in order.
  - every chapter has exactly one "## Key Takeaway" and one
    "## The Next Obvious Question" section, and exactly one
    "## Opening Question" and one "## Guided Lab" section.
  - the Opening Question is a single sentence ending in "?".
  - the Guided Lab section states a portability tag (BLUEPRINT.md Section
    13.2: portable / hardware-dependent / privileged / bare-metal
    recommended) and a fallback path.
  - figures (markdown image references) have non-empty alt text.

ERROR-level findings are the four unconditional structural requirements
above (Section 21's "every chapter has one key takeaway and one next
question", extended to Opening Question and Guided Lab, both of which
Section 25's Definition of Done treats as unconditional too). Missing
other template sections (Worked Example, Common Misconceptions, etc.) are
WARNING-level: Section 25 qualifies both ("the story and worked example
are distinct" does not mandate a separate heading; "at least two
misconceptions are addressed where relevant" is explicitly conditional),
and several drafted chapters legitimately fold a Worked Example into
their Incident/Story section or have no misconception genuinely relevant
to their content.

Exit code: 0 if no ERROR-level issue is found, 1 otherwise.
"""
import argparse
import re
import sys
from pathlib import Path

CHAPTER_FILE_RE = re.compile(r"^chapter-(\d+)-.*\.md$")
PART_DIR_RE = re.compile(r"^part-(\d+)-")
H1_RE = re.compile(r"^#\s+Chapter\s+(\d+)\s+—")
SECTION_HEADING_RE = re.compile(r"^##\s+(.+?)\s*$", re.MULTILINE)
IMAGE_RE = re.compile(r"!\[([^\]]*)\]\(([^)]+)\)")

TEMPLATE_SECTIONS = [
    "Opening Question",
    "Incident or Real-World Story",
    "Predict Before Measuring",
    "Worked Example",
    "Core Intuition",
    "Technical Explanation",
    "Tool View",
    "Guided Lab",
    "Common Misconceptions",
    "Practical Implications",
    "Key Takeaway",
    "What to Remember",
    "Further Reading",
    "The Next Obvious Question",
]
STRICT_SECTIONS = {"Opening Question", "Guided Lab", "Key Takeaway", "The Next Obvious Question"}
PORTABILITY_TAGS = ["portable", "hardware-dependent", "privileged", "bare-metal recommended"]
CHAPTERS_PER_PART = 5


def section_headings(text: str):
    return [h.strip() for h in SECTION_HEADING_RE.findall(text)]


def section_body(text: str, heading_prefix: str) -> str:
    """First section whose heading starts with heading_prefix (case-sensitive
    prefix match, so 'Worked Example' also matches 'Worked Examples' and
    'Worked Example -- subtitle')."""
    headings = list(SECTION_HEADING_RE.finditer(text))
    for i, h in enumerate(headings):
        if h.group(1).strip().startswith(heading_prefix):
            start = h.end()
            end = headings[i + 1].start() if i + 1 < len(headings) else len(text)
            return text[start:end].strip()
    return None


def count_matching(headings, heading_prefix: str) -> int:
    return sum(1 for h in headings if h.startswith(heading_prefix))


def check_filename_and_numbering(path: Path, text: str, errors):
    fm = CHAPTER_FILE_RE.match(path.name)
    if not fm:
        errors.append(f"{path}: filename does not match 'chapter-NN-slug.md'")
        return None
    file_n = int(fm.group(1))
    h1 = H1_RE.search(text)
    if not h1:
        errors.append(f"{path}: no '# Chapter N — Title' H1 heading found")
        return file_n
    h1_n = int(h1.group(1))
    if file_n != h1_n:
        errors.append(
            f"{path}: filename number ({file_n}) does not match H1 heading "
            f"number ({h1_n})"
        )
    return file_n


def check_part_order(book_dir: Path, errors):
    for part_dir in sorted(book_dir.glob("part-*")):
        if not part_dir.is_dir():
            continue
        pm = PART_DIR_RE.match(part_dir.name)
        if not pm:
            continue
        part_n = int(pm.group(1))
        expected = set(
            range((part_n - 1) * CHAPTERS_PER_PART + 1, part_n * CHAPTERS_PER_PART + 1)
        )
        found = set()
        for cf in sorted(part_dir.glob("chapter-*.md")):
            m = CHAPTER_FILE_RE.match(cf.name)
            if m:
                found.add(int(m.group(1)))
        if found != expected:
            errors.append(
                f"{part_dir}: expected chapters {sorted(expected)}, found "
                f"{sorted(found)}"
            )


def check_structural_sections(path: Path, text: str, errors, warnings):
    headings = section_headings(text)
    for section in TEMPLATE_SECTIONS:
        n = count_matching(headings, section)
        if section in STRICT_SECTIONS:
            if n != 1:
                errors.append(
                    f"{path}: expected exactly one '## {section}' section, found {n}"
                )
        else:
            if n == 0:
                warnings.append(f"{path}: no '## {section}' section (may be folded elsewhere)")
            elif n > 1:
                warnings.append(f"{path}: multiple '## {section}' sections ({n})")


def check_opening_question(path: Path, text: str, errors):
    body = section_body(text, "Opening Question")
    if body is None:
        return  # already reported by check_structural_sections
    lines = [l.strip() for l in body.splitlines() if l.strip()]
    if not lines:
        errors.append(f"{path}: 'Opening Question' section is empty")
        return
    q = lines[0]
    if not q.endswith("?"):
        errors.append(f"{path}: Opening Question does not end in '?': {q!r}")
    if q.count("?") > 1:
        errors.append(f"{path}: Opening Question looks like more than one sentence: {q!r}")


def check_guided_lab(path: Path, text: str, errors):
    body = section_body(text, "Guided Lab")
    if body is None:
        return  # already reported by check_structural_sections
    lower = body.lower()
    if not any(tag in lower for tag in PORTABILITY_TAGS):
        errors.append(
            f"{path}: Guided Lab has no portability tag "
            f"({'/'.join(PORTABILITY_TAGS)})"
        )
    if "fallback path" not in lower:
        errors.append(f"{path}: Guided Lab has no 'Fallback path'")


def check_figures(path: Path, text: str, errors):
    for alt, target in IMAGE_RE.findall(text):
        if not alt.strip():
            errors.append(f"{path}: image reference to {target!r} has empty alt text/caption")


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--book-dir", default="book")
    parser.add_argument("--template", default="templates/chapter-template.md")
    args = parser.parse_args()

    book_dir = Path(args.book_dir)
    errors, warnings = [], []

    chapter_paths = sorted(book_dir.glob("part-*/chapter-*.md"))
    if not chapter_paths:
        errors.append(f"no chapter-*.md files found under {book_dir}")

    for path in chapter_paths:
        text = path.read_text()
        check_filename_and_numbering(path, text, errors)
        check_structural_sections(path, text, errors, warnings)
        check_opening_question(path, text, errors)
        check_guided_lab(path, text, errors)
        check_figures(path, text, errors)

    check_part_order(book_dir, errors)

    for w in warnings:
        print(f"WARNING: {w}", file=sys.stderr)
    for e in errors:
        print(f"ERROR: {e}", file=sys.stderr)

    print(
        f"\nvalidate_chapter_metadata.py: {len(chapter_paths)} chapters checked, "
        f"{len(errors)} error(s), {len(warnings)} warning(s)",
        file=sys.stderr,
    )
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
