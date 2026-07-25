#!/usr/bin/env python3
"""validate_chapter_metadata.py -- TODO, not yet implemented.

Will validate, per BLUEPRINT.md Section 21 ("Validation Rules"):
  - "chapter numbering and filenames"
  - "part and chapter order"
  - "every chapter has one key takeaway and one next question"
  - "figures have captions and source metadata"

And per Section 25 ("Definition of Done for a Chapter"):
  - opening question present and single-sentence
  - all required template sections present (see templates/chapter-template.md)
  - at least one Guided Lab portability tag from BLUEPRINT.md Section 13.2
  - a Fallback Path is present for the Guided Lab

Intended approach once implemented: parse each `book/part-*/chapter-*.md`
file's headings against templates/chapter-template.md's section list,
confirm filename numbering matches declared chapter order, and confirm
exactly one "## Key Takeaway" and one "## The Next Obvious Question"
section per chapter.

Exits 0 unconditionally for now, per BLUEPRINT.md Section 21's "What CI
must not do" -- an unimplemented validator must not fail a build.
"""
import argparse
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--book-dir", default="book",
        help="path to the book's chapter directory tree (default: %(default)s)",
    )
    parser.add_argument(
        "--template", default="templates/chapter-template.md",
        help="path to the chapter template to validate against (default: %(default)s)",
    )
    parser.parse_args()

    print("TODO: not yet implemented -- see BLUEPRINT.md Section 21", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
