# Artifact

## How to reproduce

I ran the experiments on the course VM inside Docker with GCC 13.3.0. The repository does not include the Docker image used for the measurements, but the code builds on a standard Unix-like machine with GCC and Make. The Makefile uses `-O3 -lm -g -Werror`. The repository includes the PNG-writing header.

1. Build and run from the repository root:

```bash
make all
/usr/bin/time -f 'Wall time: %e s' ./sandpile.exe 1001
```

The program writes `identity.png` in the current directory. Each run replaces the previous image. On macOS, use `time ./sandpile.exe 1001` instead of the GNU time command above. Use `./sandpile.exe 101` for a quicker smoke test.

2. Check the `N=1001` output against the supplied reference. This requires Python 3 and Pillow (`python3 -m pip install Pillow`):

```bash
python3 - <<'PY'
from PIL import Image, ImageChops

with Image.open('identity.png') as result, Image.open('identity-ref.png') as reference:
    result = result.convert('RGB')
    reference = reference.convert('RGB')
    if result.size != reference.size or ImageChops.difference(result, reference).getbbox() is not None:
        raise SystemExit('Output does not match reference')
    print('Output matches reference:', result.size)
PY
```

This checks the decoded image, including the inactive region. The diagnostic builds used for the paper also compared the full integer grid after each phase; those diagnostic hooks and reference grids are not included in this repository.

3. To test a particular optimization, use a separate clone, check out its commit from the table below, rebuild, and rerun steps 1 and 2:

```bash
git clone https://github.com/yashvrao/598APE-HW3.git sandpile-comparison
cd sandpile-comparison
git checkout --detach <commit>
make clean
make all
```

Replace `<commit>` with a commit from the table. You need access to the private repository to clone it. For a fair comparison, run one version at a time on an otherwise idle machine. Repeat each version several times, alternate their order, and compare medians. Timings include image generation but not compilation.

## Optimizations

All times below are wall-clock seconds for `N=1001` on the course VM. Each optimization builds on the preceding ones.

| Version | Commit | Change |
| --- | --- | --- |
| Baseline | `15fe837445b8e32a15ac61fcfa388980b54d9173` | Original implementation |
| Row intervals | `f508b3833785b1085e0647b90ac4a7df1c8acbff` | Precompute active bounds for each row |
| Batching | `bd181e08c8a4e67322b293c4278a07930ca3c302` | Batch legal topplings |
| Symmetry | `6eaa891e0bffd343d333d41baf83cd1953abd8c5` | Process one representative half for odd sizes |
| Deferred writes | `efb21b90569aa362e6ee51b871c4f6fb1c83f2ed` | Reconstruct the mirrored half after each phase |

| Change | Control | Candidate | Speedup |
| --- | ---: | ---: | ---: |
| Row intervals | 716.62 s | 351.05 s | 2.04x |
| Batching | 323.58 s | 283.71 s | 1.14x |
| Symmetry | 280.75 s | 200.00 s | 1.40x |
| Deferred writes | 200.00 s | 148.20 s | 1.35x |

These are medians from three interleaved runs per version in separate experiments. The row-interval control included a parser fix for profiler compatibility, so it was not byte-identical to the original baseline commit. The original baseline median was 730.00 s, with a range of 667.95–730.42 s.

Row intervals avoid repeated geometry checks. Batching sends multiple legal topplings to the neighbors at once. Symmetry avoids making duplicate toppling decisions for the reflected half. Deferred writes removes repeated updates to that half and copies all rows, including inactive cells, at the end of each phase. The symmetry changes rely on the symmetric inputs in this identity computation; even sizes use the full-grid fallback.

I also tried a FIFO worklist, which was much slower at the tested sizes, and a multigrid-based approach, whose `N=1001` diagnostic timed out during recurrence correction. The timings above will vary with hardware and system load.
