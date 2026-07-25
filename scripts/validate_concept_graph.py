#!/usr/bin/env python3
"""validate_concept_graph.py -- TODO, not yet implemented.

Will validate, per BLUEPRINT.md Section 21 ("Validation Rules"):
  - "concept prerequisites"
  - "glossary terms introduced before use"
  - "Chapter N's next question matches Chapter N+1's opening question"

Intended approach once implemented: parse concept-graph.yaml's
`introduced_in_chapter` fields and `also_appears_in` notes, parse each
drafted chapter's "New concepts:" line and "Opening Question"/"The Next
Obvious Question" sections, and confirm (a) no chapter uses a Level-N
concept before it or a lower level has been introduced, except where
concept-graph.yaml explicitly marks the use as `also_appears_in`
(see concept-graph.md's "Known tensions" section for why that escape
hatch exists), and (b) the narrative-graph chain holds end to end.

Exits 0 unconditionally for now, per BLUEPRINT.md Section 21's "What CI
must not do" -- an unimplemented validator must not fail a build.
"""
import argparse
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--concept-graph", default="concept-graph.yaml",
        help="path to the machine-readable concept graph (default: %(default)s)",
    )
    parser.add_argument(
        "--book-dir", default="book",
        help="path to the book's chapter directory tree (default: %(default)s)",
    )
    parser.parse_args()

    print("TODO: not yet implemented -- see BLUEPRINT.md Section 21", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
