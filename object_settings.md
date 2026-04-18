# Object file settings

Object files are plain text read by `ObjectLoader` / `OFFLoader` (see `Include/ObjectLoader.h`). Each file starts with an optional **header** of keyword lines, then a **material** declaration, then **geometry** (format depends on file type).

## Header keywords (optional, any order)

These tokens are read in a loop until a word that is not `position`, `scale`, or `format` is seen; that word is interpreted as the material type.

### `position`

- **Syntax:** `position <x> <z> <y>`
- **Meaning:** Translation applied to every vertex after scaling. Components are stored as `(x, y, z)` in the scene, but the file order is **x, z, y** (Y and Z are swapped when reading).
- **Default:** `(0, 0, 0)` if omitted.

### `scale`

- **Syntax:** `scale <s>`
- **Meaning:** Uniform multiplier applied to vertex coordinates before adding `position`.
- **Default:** `1` if omitted.

### `format`

- **Syntax:** `format <name>`
- **Options:**

| Value | Meaning |
|-------|---------|
| `xyz` | Use coordinates as read (default). |
| `xzy` | Swap Y and Z for each vertex when loading (useful when a mesh was authored in a different axis convention). |

If an invalid value is given, the loader prints an error, resets `format` to `xyz`, and treats the line as failed.

---

## Material

Immediately after the header loop, the next token is the **material type**. Parameters follow on the same stream (whitespace-separated).

### `default-material`

- **Extra parameters:** none.
- **Effect:** Gray diffuse material with albedo `(0.75, 0.75, 0.75)` (Lambertian).

### `lambertian`

- **Syntax:** `lambertian <r> <g> <b>`
- **Meaning:** Diffuse surface with RGB albedo (typical range 0–1).

### `specular`

- **Syntax:** `specular <r> <g> <b> <roughness>`
- **Meaning:** Specular (glossy) material; `roughness` controls highlight spread (see `Material::Specular` in `Include/Materials.h`).

### `metallic`

- **Syntax:** `metallic <r> <g> <b> <roughness> <metallic>`
- **Meaning:** PBR-style metallic workflow parameters on top of base color.

### `transparent`

- **Syntax:** `transparent <r> <g> <b> <transparency> <refractionIndex>`
- **Meaning:** Glass-like material with transparency and index of refraction.

### `lightsource`

- **Syntax:** `lightsource <r> <g> <b> <luminescent>`
- **Meaning:** Emissive material; `luminescent` scales emission (defaults to `1.0` in the type, but the loader still expects a float to be read after color).

Any other material name prints `unknown material type` and loading fails for that object.

---

## Geometry (after material)

Which parser runs depends on the **filename** (see `Scene::LoadObjects` in `src/Scene.cpp`):

- Path contains `.off` or `.OFF` → **OFF** format (`OFFLoader`).
- Otherwise → **raw triangle list** (`ObjectLoader`).

### OFF files

1. Header line: `OFF`
2. Line: `<vertices count> <faces count> <edges count>`
3. `vertices count` lines: `x y z` per vertex (subject to `scale`, `position`, and `format`).
4. If `faces count` ≠ 0: for each face, a line `n a b c` (triangle with vertex indices `a`, `b`, `c`; `n` is read but the code expects triangle data).
5. If `faces count` == 0: each vertex is expanded into a small cube of triangles (point-cloud mode).

### Raw triangle files (non-OFF)

1. Integer `n`: number of triangles.
2. `n` lines, each with nine floats: `x1 y1 z1 x2 y2 z2 x3 y3 z3` for the three corners of one triangle (then `scale`, `position`, and `format` are applied as in `ObjectLoader::ExtractTriangles`).

---

## Example snippets

```
position 0 0 -2
scale 0.5
format xzy
lambertian 0.9 0.2 0.2
OFF
4 2 0
...
```

```
metallic 0.8 0.8 0.9 0.1 1.0
1
0 0 0  1 0 0  0 1 0
```

(Geometry after `lambertian` / `metallic` must match OFF vs non-OFF rules above.)
