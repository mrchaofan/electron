# LabelView

> A View that displays styled text.

Process: [Main](../glossary.md#main-process)

This module cannot be used until the `ready` event of the `app`
module is emitted.

## Class: LabelView extends `View`

> A native label view for displaying text.

Process: [Main](../glossary.md#main-process)

`LabelView` inherits from [`View`](view.md).

`LabelView` is an [EventEmitter][event-emitter].

> [!WARNING]
> Electron's built-in classes cannot be subclassed in user code.
> For more information, see [the FAQ](../faq.md#class-inheritance-does-not-work-with-electron-built-in-modules).

### `new LabelView()` _Experimental_

Creates a new `LabelView`.

### Instance Methods

The following methods are available on instances of the `LabelView` class, in
addition to those inherited from [View](view.md):

#### `view.setText(text)` _Experimental_

* `text` string

#### `view.getText()` _Experimental_

Returns `string` - The current label text.

#### `view.setFontFamily(fontFamily)` _Experimental_

* `fontFamily` string

#### `view.getFontFamily()` _Experimental_

Returns `string` - The current primary font family.

#### `view.setFontSize(fontSize)` _Experimental_

* `fontSize` Integer - Minimum value is `1`.

#### `view.getFontSize()` _Experimental_

Returns `Integer` - The current font size in pixels.

#### `view.setBold(bold)` _Experimental_

* `bold` boolean

#### `view.getBold()` _Experimental_

Returns `boolean` - Whether the label currently uses bold weight.

#### `view.setFontWeight(weight)` _Experimental_

* `weight` Integer - Numeric font weight (for example, `400`, `600`, `700`).

#### `view.getFontWeight()` _Experimental_

Returns `Integer` - The current numeric font weight.

#### `view.setTextColor(color)` _Experimental_

* `color` string - Color in Hex, RGB, ARGB, HSL, HSLA or named CSS color format.

#### `view.getTextColor()` _Experimental_

Returns `string` - Color in RGBA hex format.

#### `view.setHorizontalAlignment(alignment)` _Experimental_

* `alignment` string - One of `left`, `center`, `right`, or `start`. `start`
  aligns text to the direction of its first strong character.

Sets the horizontal position of text within the LabelView bounds.

#### `view.getHorizontalAlignment()` _Experimental_

Returns `string` - The current horizontal alignment: `left`, `center`, `right`,
or `start`.

#### `view.setVerticalAlignment(alignment)` _Experimental_

* `alignment` string - One of `top`, `center`, or `bottom`.

Sets the vertical position of text within the LabelView bounds.

#### `view.getVerticalAlignment()` _Experimental_

Returns `string` - The current vertical alignment: `top`, `center`, or `bottom`.

[event-emitter]: https://nodejs.org/api/events.html#events_class_eventemitter
