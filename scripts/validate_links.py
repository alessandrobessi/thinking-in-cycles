#!/usr/bin/env python3
"""validate_links.py -- TODO, not yet implemented.

Will validate, per BLUEPRINT.md Section 21 ("Validation Rules"):
  - "internal links"
  - "referenced lab paths"

Intended approach once implemented: walk every Markdown file under
book/, templates/, references/, and the top-level registries
(glossary.md, misconceptions.md, analogy-registry.md, concept-graph.md),
extract relative links and any path referenced in a fenced code block
(e.g. `labs/scripts/ch1_time_accounting.sh`), and confirm each resolves
to a real file in the repository.

Exits 0 unconditionally for now, per BLUEPRINT.md Section 21's "What CI
must not do" -- an unimplemented validator must not fail a build.
"""
import argparse
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root", default=".",
        help="repository root to scan (default: %(default)s)",
    )
    parser.parse_args()

    print("TODO: not yet implemented -- see BLUEPRINT.md Section 21", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
