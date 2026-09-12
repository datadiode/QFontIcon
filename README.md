# QFontIcon

![AI generated description](https://github.com/user-attachments/assets/cf23e6cc-987b-470f-bd61-1a611310335a)

## Description

QFontIcon is a simple library that allows you to load one or several icon/glyph fonts and use them.

Inspired by [QtAwesome](https://github.com/gamecreature/QtAwesome).

## Key features

- Supports multiple fonts
- Supports stateful icons (`QIcon::Mode` / `QIcon::State`)
- Supports spinning animations

## CMake

It's a cmake based project you can add as a subdirectory.

## How it works
![AI generated description](https://camo.githubusercontent.com/293f979ef4266928b2dd2bd9f7055f23ed8882184493b4f9af0e1dbc20995d24/68747470733a2f2f696d672e736869656c64732e696f2f62616467652f2545322538342542392545462542382538462d414925323047656e6572617465642532304465736372697074696f6e2d677265656e3f7374796c653d666f722d7468652d6261646765)

This library implements a Qt-based system for rendering **font glyphs as `QIcon`s** (icon fonts like Font Awesome). It has three main pieces: a helper `StateMap`, the private data class `QFontIconEnginePrivate`, and the public `QFontIconEngine` (a `QIconEngine` subclass). Let me walk through each.

### 1. StateMap&lt;T&gt; — State-aware storage

```cpp
template<typename T>
class StateMap : public QMap<QPair<QIcon::Mode, QIcon::State>, T>
```

Every icon property (glyph, font, color, scale, speed, etc.) can vary based on the icon's **`QIcon::Mode`** (Normal/Active/Disabled/Selected) and **`QIcon::State`** (On/Off). `StateMap` is just a `QMap` keyed on that `(Mode, State)` pair.

The clever part is `get()`, which implements a **fallback chain**:
1. Try the exact `(Mode, State)` requested.
2. If missing, fall back to `(Normal, State)`.
3. If still missing, fall back to `(Normal, Off)` — the default state.
4. Otherwise return a supplied default value.

This means you only need to set properties for the states that differ; everything else inherits from the `(Normal, Off)` baseline.

### 2. QFontIconEnginePrivate — The internal data (PIMPL)

This holds all the actual state, keeping the public header clean:

- **`StateMap`s** for `icons`, `fonts`, `scales`, `colors`, `speeds`, `curves`, plus animation state (`progress`, `angles`).
- **`availableFonts`** — a static registry mapping a font name (URL fragment) to a `QRawFont` (the loaded font) and a `QMetaEnum` (which maps glyph *names* like `"super-glyph"` to code points).

Key helpers:

- **`setupTimer()`** — Drives the spinning animation. A single **static `QTimer`** shared across all engines fires every 20 ms. A reference count (`timerRefCount`) starts/stops it. On each tick, for every state with a nonzero `speed` (degrees/second), it advances `progress` (0–1), applies the easing `curve`, converts to an `angle` (0–360°), and calls `widget->update()` to trigger a repaint.

- **`getFont()` / `getEnum()`** — Look up the `QRawFont` / `QMetaEnum` from the registry.

- **`resizeFont()`** — Sizes the glyph to fit the draw rectangle, normalizing so different glyphs render at consistent size using `maxCharWidth()`.

- **`keyToValue()`** — A **binary search** replacement for `QMetaEnum::keyToValue()` (which is linear). It requires the enum keys to be sorted (produced by a `generate_fa.py` script). The `ok` flag comment (`// it's cool man`) hints it's an intentionally tricky one-liner.

### 3. QFontIconEngine — The public QIconEngine

This is what plugs into `QIcon`. Constructors let you build an icon from:
- a code point (`int`) + font name,
- a glyph name (`QString`) + font name,
- or a **`QUrl`** — a compact declarative form.

#### The QUrl constructor (the most interesting one)

It parses a URL like `":/fonts/fa.ttf?...#fa.ttf"`:
- **No query** → treat it as a font-*load* request: load the TTF file into `availableFonts` under the fragment name.
- **With query** → configure per-state properties. Each query key is a **bitmask** (bits `0x01..0x08` = Off states per mode, `0x10..0x80` = On states):
  - A **plain numeric key** sets a **color**. The value can be a named color or a hex form, and it decodes several shorthand formats by string length: `0xRGB`, `0xRRGGBB`, `0xARGB`, `0xAARRGGBB` (expanding short forms by multiplying nibbles, e.g. `0x11`).
  - A **`.codepoint`** suffix sets the **glyph** for the masked states, accepting either a numeric code point or a name (resolved via `keyToValue`).

#### Getters / setters

`icon()`, `font()`, `color()`, `scale()`, `speed()`, `curve()`, etc. all read from the private `StateMap`s using the fallback logic. Notably `color()` falls back to the application palette's `ButtonText` color appropriate to the mode when none is set. The setters write into the maps; `setSpeed()` and `setWidget()` also call `setupTimer()`.

#### Rendering — paint()

This is where a glyph becomes pixels:
1. Look up the glyph index, font, scale, and color for the state.
2. Resize the font to the rectangle.
3. If there's a rotation `angle` (from the animation), translate to center, rotate, translate back.
4. Get the glyph as a **`QPainterPath`** (`pathForGlyph`) and fill it with the color — so it's vector-drawn, not bitmap text.
5. Optionally draw a red **badge** (a small ellipse in the top-right corner).

`pixmap()` just paints onto a transparent `QPixmap`.

#### Loading & factory helpers

- **`loadFont()`** registers a TTF file with an optional `QMetaEnum` for name lookups.
- Static **`icon(...)`** overloads are convenience wrappers creating a `QIcon(new QFontIconEngine(...))`.
- **`QFontIconPlugin::create()`** is the Qt plugin entry point: when Qt asks for an icon whose name ends in `.ttf`, it returns a `QFontIconEngine`. It also resolves a static-vs-dynamic plugin conflict by preferring an already-registered child plugin.

### Summary

The design lets you declare icons entirely via strings/URLs (great for `.qrc` and stylesheets), supports **per-state** customization with sensible fallbacks, renders glyphs as crisp vector paths, and offers optional **spin animation** and **badges** — all through the standard `QIconEngine` interface so the result is just a normal `QIcon`.