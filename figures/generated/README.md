# figures/generated

Rendered figure assets (PNG/SVG output from `figures/source/`) as used by
the Quarto build in `publish/`. Checked into version control like any
other book asset, not treated as disposable build output — see the
root `.gitignore`, which deliberately does not exclude this directory.

**Mostly empty**, alongside `figures/source/` — see that directory's
README for why. Two real files live here:

- `ch14-flame-graph-example.svg`, a genuine captured-and-rendered flame
  graph (`cyclelab compute --threads=2 --chains=4 --op=mixed`, sampled
  with macOS `sample`, folded and rendered by
  `labs/scripts/foldstacks.py` and `flamegraph_svg.py`), used in
  Chapter 14 since that chapter is specifically about reading a visual
  artifact.
- `cover.png`, the book's cover artwork, referenced by `_quarto.yml`'s
  `book.cover-image` key (used directly as the EPUB cover; shown on the
  HTML site's landing page depending on theme).
