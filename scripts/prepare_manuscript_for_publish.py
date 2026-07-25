#!/usr/bin/env python3
"""prepare_manuscript_for_publish.py -- TODO, not yet implemented.

Will eventually assemble the manuscript for a Quarto build (see
publish/_quarto.yml and BLUEPRINT.md Section 26, "Definition of Done for
the Book": "the HTML, PDF, and EPUB builds succeed"). Intended
responsibilities once implemented:
  - confirm every chapter listed in publish/_quarto.yml exists on disk
  - run validate_concept_graph.py, validate_chapter_metadata.py, and
    validate_links.py first and refuse to proceed if any reports errors
    (once those are themselves implemented -- today they are stubs)
  - copy/refresh any generated figures from figures/source into
    figures/generated as needed by the current chapter set
  - invoke `quarto render` from publish/

Exits 0 unconditionally for now, per BLUEPRINT.md Section 21's "What CI
must not do" -- an unimplemented step must not fail a build.
"""
import argparse
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--quarto-dir", default="publish",
        help="path to the Quarto project directory (default: %(default)s)",
    )
    parser.parse_args()

    print("TODO: not yet implemented -- see BLUEPRINT.md Section 26", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
