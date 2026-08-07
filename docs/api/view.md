# View

> Create and layout native views.

Process: [Main](../glossary.md#main-process)

This module cannot be used until the `ready` event of the `app`
module is emitted.

```js
const { BaseWindow, View } = require('electron')

const win = new BaseWindow()
const view = new View()

view.setBackgroundColor('red')
view.setBounds({ x: 0, y: 0, width: 100, height: 100 })
win.contentView.addChildView(view)
```

## Class: View

> A basic native view.

Process: [Main](../glossary.md#main-process)

`View` is an [EventEmitter][event-emitter].

> [!WARNING]
> Electron's built-in classes cannot be subclassed in user code.
> For more information, see [the FAQ](../faq.md#class-inheritance-does-not-work-with-electron-built-in-modules).

### `new View()`

Creates a new `View`.

### Instance Events

Objects created with `new View` emit the following events:

#### Event: 'bounds-changed'

Emitted when the view's bounds have changed in response to being laid out. The
new bounds can be retrieved with [`view.getBounds()`](#viewgetbounds).

#### Event: 'mouse-pressed' _Experimental_

Returns an Object containing `location` ([Point](structures/point.md)),
`rootLocation` ([Point](structures/point.md)), `flags` (Integer),
`changedButtonFlags` (Integer), `clickCount` (Integer), and `returnValue`
(boolean), which defaults to `false`. Set `returnValue` to `true` to allow the
native view to handle the press. Calling `preventDefault()` always prevents
native handling.

#### Event: 'mouse-dragged' _Experimental_

Returns an Object containing `location` ([Point](structures/point.md)),
`rootLocation` ([Point](structures/point.md)), `flags` (Integer),
`changedButtonFlags` (Integer), `clickCount` (Integer), and `returnValue`
(boolean), which defaults to `false`. Set `returnValue` to `true` to allow the
native view to handle the drag. Calling `preventDefault()` always prevents
native handling.

#### Event: 'mouse-released' _Experimental_

Returns an Object containing `location` ([Point](structures/point.md)),
`rootLocation` ([Point](structures/point.md)), `flags` (Integer),
`changedButtonFlags` (Integer), and `clickCount` (Integer).

#### Event: 'mouse-capture-lost' _Experimental_

Emitted when the view loses mouse capture.

#### Event: 'mouse-moved' _Experimental_

Returns an Object containing `location` ([Point](structures/point.md)),
`rootLocation` ([Point](structures/point.md)), `flags` (Integer),
`changedButtonFlags` (Integer), and `clickCount` (Integer).

#### Event: 'mouse-entered' _Experimental_

Returns an Object containing `location` ([Point](structures/point.md)),
`rootLocation` ([Point](structures/point.md)), `flags` (Integer),
`changedButtonFlags` (Integer), and `clickCount` (Integer).

#### Event: 'mouse-exited' _Experimental_

Returns an Object containing `location` ([Point](structures/point.md)),
`rootLocation` ([Point](structures/point.md)), `flags` (Integer),
`changedButtonFlags` (Integer), and `clickCount` (Integer). This event is not
emitted merely because the pointer enters a child view; it is emitted after the
pointer leaves the entire View subtree.

#### Event: 'mouse-wheel' _Experimental_

Returns an Object containing `location` ([Point](structures/point.md)),
`rootLocation` ([Point](structures/point.md)), `flags` (Integer),
`changedButtonFlags` (Integer), `clickCount` (Integer), `offsetX` (Integer),
`offsetY` (Integer), and `returnValue` (boolean), which defaults to `true`.
Set `returnValue` to `false` or call `preventDefault()` to prevent the native
view from handling the event.

#### Event: 'hit-test-point' _Experimental_

Returns an Object containing `location` ([Point](structures/point.md)) and
`returnValue` (boolean), which defaults to `true`. This event is emitted while
testing whether a point targets the view. Set `returnValue` to `false` or call
`preventDefault()` to make the point miss the view.

#### Event: 'layout' _Experimental_

Returns an Object containing `width` (Integer) and `height` (Integer).
This is a notification event; it does not expose `returnValue`, and the View's
default Chromium layout implementation always runs.

### Instance Methods

Objects created with `new View` have the following instance methods:

#### `view.addChildView(view[, index])`

* `view` View - Child view to add.
* `index` Integer (optional) - Index at which to insert the child view.
  Defaults to adding the child at the end of the child list.

If the same View is added to a parent which already contains it, it will be reordered such that
it becomes the topmost view.

#### `view.removeChildView(view)`

* `view` View - Child view to remove.

If the view passed as a parameter is not a child of this view, this method is a no-op.

#### `view.setBounds(bounds[, options])`

* `bounds` [Rectangle](structures/rectangle.md) - New bounds of the View.
* `options` Object (optional) - Options for setting the bounds.
  * `animate` boolean | Object (optional) - If true, the bounds change will be animated. If an object is passed, it can contain the following properties:
    * `duration` Integer (optional) - Duration of the animation in milliseconds. Default is `250`.
    * `easing` string (optional) - Easing function for the animation. Default is `linear`.
      * `linear`
      * `ease-in`
      * `ease-out`
      * `ease-in-out`

#### `view.getBounds()`

Returns [`Rectangle`](structures/rectangle.md) - The bounds of this View, relative to its parent.

#### `view.getLocalBounds()`

Returns [`Rectangle`](structures/rectangle.md) - The bounds of this View in its own coordinate space.

#### `view.setBackgroundColor(color)`

* `color` string - Color in Hex, RGB, ARGB, HSL, HSLA or named CSS color format. The alpha channel is
  optional for the hex type.

Examples of valid `color` values:

* Hex
  * `#fff` (RGB)
  * `#ffff` (ARGB)
  * `#ffffff` (RRGGBB)
  * `#ffffffff` (AARRGGBB)
* RGB
  * `rgb\(([\d]+),\s*([\d]+),\s*([\d]+)\)`
    * e.g. `rgb(255, 255, 255)`
* RGBA
  * `rgba\(([\d]+),\s*([\d]+),\s*([\d]+),\s*([\d.]+)\)`
    * e.g. `rgba(255, 255, 255, 1.0)`
* HSL
  * `hsl\((-?[\d.]+),\s*([\d.]+)%,\s*([\d.]+)%\)`
    * e.g. `hsl(200, 20%, 50%)`
* HSLA
  * `hsla\((-?[\d.]+),\s*([\d.]+)%,\s*([\d.]+)%,\s*([\d.]+)\)`
    * e.g. `hsla(200, 20%, 50%, 0.5)`
* Color name
  * Options are listed in [SkParseColor.cpp](https://source.chromium.org/chromium/chromium/src/+/main:third_party/skia/src/utils/SkParseColor.cpp;l=11-152;drc=eea4bf52cb0d55e2a39c828b017c80a5ee054148)
  * Similar to CSS Color Module Level 3 keywords, but case-sensitive.
    * e.g. `blueviolet` or `red`

> [!NOTE]
> Hex format with alpha takes `AARRGGBB` or `ARGB`, _not_ `RRGGBBAA` or `RGB`.

#### `view.setBorderRadius(radius)`

* `radius` Integer - Border radius size in pixels.

> [!NOTE]
> The area cutout of the view's border still captures clicks.

#### `view.setMasksToBounds(masksToBounds)`

* `masksToBounds` boolean - Whether the View clips its content and descendants to
  its bounds.

Creates a composited layer if needed, then sets its `masksToBounds` property.

#### `view.setBorder(options)`

* `options` Object | null - Set to `null` to remove the view's border.
  * `thickness` Integer (optional) - Border thickness in pixels. Default is `1`. A value of `0` or less removes the border.
  * `radius` number (optional) - Border corner radius in pixels. Default is `0`.
  * `color` string (optional) - Border color in Hex, RGB, ARGB, HSL, HSLA or named CSS color format. Default is `black`. See [`view.setBackgroundColor(color)`](#viewsetbackgroundcolorcolor) for accepted formats.

Only a solid border style is currently supported.

#### `view.setBackgroundBlur(blurRadius)`

* `blurRadius` Integer - The radius of the background blur effect (in pixels).

> [!NOTE]
> You must set a background color with an alpha channel (e.g. `#80ffffff`) in order for the blur effect to be visible.

#### `view.setVisible(visible)`

* `visible` boolean - If false, the view will be hidden from display.

#### `view.getVisible()`

Returns `boolean` - Whether the view should be drawn. Note that this is
different from whether the view is visible on screen—it may still be obscured
or out of view.

#### `view.getPreferredSize()`

Returns [`Size`](structures/size.md) - The preferred size of this View.

#### `view.setPreferredSize(size)`

* `size` [Size](structures/size.md) | null - The preferred size to set for this View.

Sets the preferred size of this View. Pass `null` to clear a previously set preferred size.

#### `view.sizeToContents()`

Resizes this View to match its preferred size.

#### `view.calculatePreferredSize(availableSize)` _Experimental_

* `availableSize` [Size](structures/size.md) - The space available to the View.
  `width` and `height` are `Infinity` when unconstrained.

Returns [`Size`](structures/size.md) - Override this method to customize the value
returned by `view.getPreferredSize()`. The default implementation returns an
empty size.

#### `view.setTranslation(x[, y])` _Experimental_

* `x` number
* `y` number (optional) - Defaults to `0`.

Translates the View's composited layer. Repeated calls accumulate transforms.

#### `view.setScale(x[, y])` _Experimental_

* `x` number
* `y` number (optional) - Defaults to `x`.

Scales the View's composited layer. Repeated calls accumulate transforms.

#### `view.setRotation(degrees)` _Experimental_

* `degrees` number

Rotates the View's composited layer. Repeated calls accumulate transforms.

#### `view.resetTransform()` _Experimental_

Resets the View's composited-layer transform to identity.

#### `view.setLayerOpacity(opacity)` _Experimental_

* `opacity` number - Clamped to the range `[0, 1]`.

Creates a composited layer if needed, then sets its opacity.

#### `view.setShadow(options)` _Experimental_

* `options` Object
  * `color` string (optional) - CSS color. Defaults to `rgba(0, 0, 0, 0.25)`.
  * `offsetX` Integer (optional) - Horizontal offset in pixels. Defaults to `0`.
  * `offsetY` Integer (optional) - Vertical offset in pixels. Defaults to `0`.
  * `blurRadius` Integer (optional) - Non-negative blur radius in pixels. Defaults to `0`.
  * `spreadRadius` Integer (optional) - Spread distance in pixels. Defaults to `0`.
  * `borderRadius` Integer (optional) - Corner radius in pixels. Defaults to the
    current `view.setBorderRadius()` value, or `0` when unset.

Creates or updates a CSS-style outer shadow in an independent composited layer
below the View. The shadow follows the View's bounds and layer transforms.

#### `view.clearShadow()` _Experimental_

Removes the current CSS-style shadow layer from the View.

#### `view.invalidateLayout()` _Experimental_

Invalidates this View's layout. A new layout is performed asynchronously.

#### `view.schedulePaint()` _Experimental_

Schedules this View to be repainted.

#### `view.preferredSizeChanged()` _Experimental_

Notifies this View's parent that its preferred size has changed and invalidates
layout as needed.

### Instance Properties

Objects created with `new View` have the following properties:

#### `view.children` _Readonly_

A `View[]` property representing the child views of this view.

[event-emitter]: https://nodejs.org/api/events.html#events_class_eventemitter
