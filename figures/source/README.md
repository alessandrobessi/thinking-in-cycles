# figures/source

Editable diagram sources (e.g. Mermaid, SVG, or vector-drawing project
files) for the book's diagrams, per BLUEPRINT.md Section 7.6: "no
decorative diagrams... every diagram must answer one explicit question."

**Still mostly empty.** Chapters 1-13 and 15 are prose-only by deliberate
decision — no diagrams were added, to avoid dangling figure references
before a real diagramming pass exists. Chapter 14 is the one exception:
it uses a real, captured flame graph (`figures/generated/ch14-flame-graph-example.svg`)
rendered directly by `labs/scripts/flamegraph_svg.py`, not a hand-drawn
diagram, so there is no corresponding editable "source" file here for
it. Further candidates, once a real diagramming pass starts, include
Chapter 1's elapsed-time accounting (on-CPU vs. off-CPU), Chapter 3's
checkout-line saturation diagram, and Chapter 8's pipeline/dependency-
chain picture (the assembly-line analogy is already used in prose; a
diagram is a natural next step).
