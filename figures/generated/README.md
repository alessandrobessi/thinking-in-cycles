# figures/generated

Rendered figure assets (PNG/SVG output from `figures/source/`) as used by
the Quarto build in `publish/`. Checked into version control like any
other book asset, not treated as disposable build output — see the
root `.gitignore`, which deliberately does not exclude this directory.

**Mostly empty**, alongside `figures/source/` — see that directory's
README for why. One real file lives here:
`ch14-flame-graph-example.svg`, a genuine captured-and-rendered flame
graph (`cyclelab compute --threads=2 --chains=4 --op=mixed`, sampled
with macOS `sample`, folded and rendered by
`labs/scripts/foldstacks.py` and `flamegraph_svg.py`), used in Chapter
14 since that chapter is specifically about reading a visual artifact.
