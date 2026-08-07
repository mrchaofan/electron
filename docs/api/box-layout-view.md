# BoxLayoutView

> A View backed by Chromium's BoxLayoutView.

Process: [Main](../glossary.md#main-process)

This module cannot be used until the `ready` event of the `app`
module is emitted.

## Class: BoxLayoutView extends `View`

> A native container view with configurable box layout behavior.

Process: [Main](../glossary.md#main-process)

`BoxLayoutView` inherits from [`View`](view.md).

`BoxLayoutView` is an [EventEmitter][event-emitter].

> [!WARNING]
> Electron's built-in classes cannot be subclassed in user code.
> For more information, see [the FAQ](../faq.md#class-inheritance-does-not-work-with-electron-built-in-modules).

### `new BoxLayoutView()` _Experimental_

Creates a new `BoxLayoutView`.

### Instance Methods

The following methods are available on instances of the `BoxLayoutView` class,
in addition to those inherited from [View](view.md):

#### `view.setOrientation(orientation)` _Experimental_

* `orientation` string - Either `horizontal` or `vertical`.

#### `view.getOrientation()` _Experimental_

Returns `string` - Either `horizontal` or `vertical`.

#### `view.setMainAxisAlignment(alignment)` _Experimental_

* `alignment` string - One of `start`, `center`, `end`, `stretch`, `baseline`.

#### `view.getMainAxisAlignment()` _Experimental_

Returns `string` - One of `start`, `center`, `end`, `stretch`, `baseline`.

#### `view.setCrossAxisAlignment(alignment)` _Experimental_

* `alignment` string - One of `start`, `center`, `end`, `stretch`, `baseline`.

#### `view.getCrossAxisAlignment()` _Experimental_

Returns `string` - One of `start`, `center`, `end`, `stretch`, `baseline`.

#### `view.setInsideBorderInsets(insets)` _Experimental_

* `insets` Object
  * `top` Integer
  * `left` Integer
  * `bottom` Integer
  * `right` Integer

#### `view.getInsideBorderInsets()` _Experimental_

Returns `Object`:

* `top` Integer
* `left` Integer
* `bottom` Integer
* `right` Integer

#### `view.setBetweenChildSpacing(spacing)` _Experimental_

* `spacing` Integer

#### `view.getBetweenChildSpacing()` _Experimental_

Returns `Integer` - The spacing between child views.

#### `view.setCollapseMarginsSpacing(collapse)` _Experimental_

* `collapse` boolean

#### `view.getCollapseMarginsSpacing()` _Experimental_

Returns `boolean` - Whether adjacent margins and spacing collapse.

#### `view.setDefaultFlex(flex)` _Experimental_

* `flex` Integer

#### `view.getDefaultFlex()` _Experimental_

Returns `Integer` - The default flex value.

#### `view.setFlexForView(view, flex[, useMinSize])` _Experimental_

* `view` View - The child view to set the flex weight for.
* `flex` Integer - The flex weight. A flex of `0` means this view is not
  resized when extra space is available.
* `useMinSize` boolean (optional) - If true, use the view's minimum size when
  laying out. Defaults to `false`.

#### `view.clearFlexForView(view)` _Experimental_

* `view` View - The child view to clear the flex weight for.

[event-emitter]: https://nodejs.org/api/events.html#events_class_eventemitter
