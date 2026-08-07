# ScrollView

> A View backed by Chromium's ScrollView.

Process: [Main](../glossary.md#main-process)

This module cannot be used until the `ready` event of the `app`
module is emitted.

## Class: ScrollView extends `View`

> A native container view that makes its content view scrollable.

Process: [Main](../glossary.md#main-process)

`ScrollView` inherits from [`View`](view.md).

`ScrollView` is an [EventEmitter][event-emitter].

> [!WARNING]
> Electron's built-in classes cannot be subclassed in user code.
> For more information, see [the FAQ](../faq.md#class-inheritance-does-not-work-with-electron-built-in-modules).

### `new ScrollView()` _Experimental_

Creates a new `ScrollView`.

### Instance Events

Objects created with `new ScrollView` emit the following events:

#### Event: 'scroll' _Experimental_

Use `view.getScrollOffset()` inside the listener to read the current offset.

### Instance Methods

The following methods are available on instances of the `ScrollView` class,
in addition to those inherited from [View](view.md):

#### `view.setContentView(view)` _Experimental_

* `view` View - The view to make scrollable.

#### `view.getContentView()` _Experimental_

Returns `View | null` - The current content view, or `null` if none has
been set.

#### `view.getVisibleRect()` _Experimental_

Returns `Rectangle` - The currently visible region of the content view, in
the content view's coordinate space.

#### `view.getScrollOffset()` _Experimental_

Returns [Point](structures/point.md) - The current content scroll offset.

#### `view.scrollToOffset(x, y)` _Experimental_

* `x` Integer
* `y` Integer

Scrolls the content view to the given offset.

#### `view.scrollByOffset(x, y)` _Experimental_

* `x` Integer
* `y` Integer

Scrolls the content view by the given offset.

#### `view.scrollToView(view)` _Experimental_

* `view` View - A descendant of the content view to scroll into view.

Scrolls the minimum amount necessary to make `view` visible.

#### `view.setHorizontalScrollBarMode(mode)` _Experimental_

* `mode` string - One of `disabled`, `hidden`, `enabled`.

#### `view.getHorizontalScrollBarMode()` _Experimental_

Returns `string` - One of `disabled`, `hidden`, `enabled`.

#### `view.setVerticalScrollBarMode(mode)` _Experimental_

* `mode` string - One of `disabled`, `hidden`, `enabled`.

#### `view.getVerticalScrollBarMode()` _Experimental_

Returns `string` - One of `disabled`, `hidden`, `enabled`.

[event-emitter]: https://nodejs.org/api/events.html#events_class_eventemitter
