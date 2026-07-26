#!/usr/bin/env python3
"""validate_concept_graph.py -- checks chapters against the concept graph
and misconception registry (BLUEPRINT.md Section 21, "Validation Rules"):

  - "concept prerequisites": every "(Chapter N)" reference in a chapter's
    **Prerequisites:** line must name a chapter strictly earlier than the
    chapter making the claim.
  - "glossary terms introduced before use": for every concept-graph.yaml
    term with a non-null `introduced_in_chapter`, glossary.md's matching
    entry (by heading, case-insensitive) must cite the same chapter in
    its "First introduced"/"Formal definition" tag.
  - "misconception IDs": every "M<N>" token referenced in a chapter's body
    must exist as a "### M<N>" heading in misconceptions.md; misconceptions.md
    itself must have no duplicate or gapped ID.
  - "Chapter N's next question matches Chapter N+1's opening question":
    walks every drafted chapter and confirms the verbatim chain holds.

Deliberately NOT implemented: a full per-term dependency DAG across
concept-graph.yaml's Level 0-7 lattice. concept-graph.yaml's own header
comment says this schema "encodes level-ordering only, not a per-term
dependency DAG" -- Level 1 (scheduling) concepts are legitimately
introduced in Chapters 21-22, well after Level 2-4 chapters, because the
book's *narrative* order and its *concept-level* order are deliberately
different (see BLUEPRINT.md Section 24's drafting-by-concept-dependency
rule vs. Section 12's narrative graph). The per-chapter Prerequisites
check above is the tractable, chapter-accurate version of "concept
prerequisites" this script actually implements.

Exit code: 0 if no issues found, 1 if any ERROR-level issue is found.
WARNING-level issues (surface-form mismatches the book already documents
as intentional) are printed but do not affect the exit code.
"""
import argparse
import re
import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    print("PyYAML is required (pip install pyyaml)", file=sys.stderr)
    sys.exit(1)

CHAPTER_FILE_RE = re.compile(r"chapter-(\d+)-.*\.md$")
PREREQ_LINE_RE = re.compile(r"^\*\*Prerequisites:\*\*\s*(.*)$", re.MULTILINE)
CHAPTER_REF_RE = re.compile(r"Chapters?\s+(\d+)(?:\s*[-–]\s*(\d+))?")
GLOSSARY_HEADING_RE = re.compile(r"^###\s+(.+?)\s*$", re.MULTILINE)
GLOSSARY_TAG_RE = re.compile(
    r"\*\*(?:First introduced|Formal definition):\*\*[^\n]*?Chapter\s+(\d+)"
)
MISCONCEPTION_HEADING_RE = re.compile(r"^###\s+(M\d+)\b", re.MULTILINE)
MISCONCEPTION_REF_RE = re.compile(r"\bM(\d{2,3})\b")


def load_chapters(book_dir: Path):
    """Returns {chapter_number: (path, text)} for every chapter-*.md file."""
    chapters = {}
    for path in sorted(book_dir.glob("part-*/chapter-*.md")):
        m = CHAPTER_FILE_RE.search(path.name)
        if not m:
            continue
        n = int(m.group(1))
        chapters[n] = (path, path.read_text())
    return chapters


def extract_section(text: str, heading: str) -> str:
    """Returns the body text of the first '## <heading>' section, or ''."""
    pattern = re.compile(
        r"^##\s+" + re.escape(heading) + r"\s*\n(.*?)(?=\n##\s+|\Z)",
        re.MULTILINE | re.DOTALL,
    )
    m = pattern.search(text)
    if not m:
        return ""
    return m.group(1).strip()


def first_prose_line(section_body: str) -> str:
    for line in section_body.splitlines():
        line = line.strip()
        if line:
            return line
    return ""


def check_prerequisites(chapters, errors, warnings):
    for n, (path, text) in sorted(chapters.items()):
        m = PREREQ_LINE_RE.search(text)
        if not m:
            errors.append(f"{path}: no '**Prerequisites:**' line found")
            continue
        line = m.group(1)
        if n == 1:
            continue  # Chapter 1 has no prerequisites by design
        refs = CHAPTER_REF_RE.findall(line)
        if not refs:
            if "entire book" in line.lower():
                continue  # Chapter 30's synthesis-style prerequisites line
            warnings.append(
                f"{path}: Prerequisites line has no '(Chapter N)' reference "
                f"to verify: {line!r}"
            )
            continue
        for lo, hi in refs:
            cited = [int(lo)] + ([int(hi)] if hi else [])
            for c in cited:
                if c >= n:
                    errors.append(
                        f"{path}: Prerequisites cites Chapter {c}, which is "
                        f"not earlier than this chapter ({n})"
                    )


def check_narrative_chain(chapters, errors):
    for n in sorted(chapters):
        path, text = chapters[n]
        opening = first_prose_line(extract_section(text, "Opening Question"))
        if not opening:
            errors.append(f"{path}: no 'Opening Question' content found")
        next_q = first_prose_line(
            extract_section(text, "The Next Obvious Question")
        )
        if not next_q:
            errors.append(f"{path}: no 'The Next Obvious Question' content found")
            continue
        successor = n + 1
        if successor not in chapters:
            continue  # Chapter 30 has no successor; nothing to chain to
        succ_path, succ_text = chapters[successor]
        succ_opening = first_prose_line(
            extract_section(succ_text, "Opening Question")
        )
        if next_q.strip() != succ_opening.strip():
            errors.append(
                f"{path}: 'Next Obvious Question' ({next_q!r}) does not "
                f"match Chapter {successor}'s Opening Question "
                f"({succ_opening!r})"
            )


def load_glossary(glossary_path: Path):
    """Returns {lowercased term: chapter_int_or_None}."""
    text = glossary_path.read_text()
    headings = list(GLOSSARY_HEADING_RE.finditer(text))
    result = {}
    for i, h in enumerate(headings):
        term = h.group(1).strip()
        start = h.end()
        end = headings[i + 1].start() if i + 1 < len(headings) else len(text)
        body = text[start:end]
        tag = GLOSSARY_TAG_RE.search(body)
        result[term.lower()] = int(tag.group(1)) if tag else None
    return result


def check_glossary_sync(concept_graph, glossary_terms, errors, warnings):
    for level in concept_graph.get("levels", []):
        for concept in level.get("concepts", []):
            name = concept["name"]
            introduced = concept.get("introduced_in_chapter")
            if introduced is None:
                continue
            key = name.lower()
            if key not in glossary_terms:
                warnings.append(
                    f"concept-graph.yaml: '{name}' (Level {level['level']}, "
                    f"Chapter {introduced}) has no matching glossary.md "
                    f"heading -- likely introduced there under a different "
                    f"surface form (see the concept's own 'note' field)"
                )
                continue
            glossary_chapter = glossary_terms[key]
            if glossary_chapter is not None and glossary_chapter != introduced:
                errors.append(
                    f"'{name}': concept-graph.yaml says Chapter {introduced}, "
                    f"glossary.md says Chapter {glossary_chapter}"
                )


def check_misconceptions(chapters, misconceptions_path: Path, errors):
    text = misconceptions_path.read_text()
    ids = MISCONCEPTION_HEADING_RE.findall(text)
    seen = set()
    for mid in ids:
        if mid in seen:
            errors.append(f"misconceptions.md: duplicate heading '### {mid}'")
        seen.add(mid)
    numbers = sorted(int(mid[1:]) for mid in seen)
    if numbers:
        expected = list(range(1, numbers[-1] + 1))
        missing = sorted(set(expected) - set(numbers))
        if missing:
            errors.append(
                f"misconceptions.md: gap in M-number sequence, missing "
                f"{['M%02d' % m for m in missing]}"
            )

    for n, (path, ctext) in sorted(chapters.items()):
        for ref in set(MISCONCEPTION_REF_RE.findall(ctext)):
            mid = f"M{ref}"
            if mid not in seen:
                errors.append(
                    f"{path}: references '{mid}', which has no "
                    f"'### {mid}' heading in misconceptions.md"
                )


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--concept-graph", default="concept-graph.yaml")
    parser.add_argument("--book-dir", default="book")
    parser.add_argument("--glossary", default="glossary.md")
    parser.add_argument("--misconceptions", default="misconceptions.md")
    args = parser.parse_args()

    concept_graph = yaml.safe_load(Path(args.concept_graph).read_text())
    chapters = load_chapters(Path(args.book_dir))
    glossary_terms = load_glossary(Path(args.glossary))

    errors, warnings = [], []

    if not chapters:
        errors.append(f"no chapter-*.md files found under {args.book_dir}")
    else:
        check_prerequisites(chapters, errors, warnings)
        check_narrative_chain(chapters, errors)
        check_glossary_sync(concept_graph, glossary_terms, errors, warnings)
        check_misconceptions(chapters, Path(args.misconceptions), errors)

    for w in warnings:
        print(f"WARNING: {w}", file=sys.stderr)
    for e in errors:
        print(f"ERROR: {e}", file=sys.stderr)

    print(
        f"\nvalidate_concept_graph.py: {len(chapters)} chapters checked, "
        f"{len(errors)} error(s), {len(warnings)} warning(s)",
        file=sys.stderr,
    )
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
