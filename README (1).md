# Ctrl + Click Copy / Double Click Paste

A [Windhawk](https://windhawk.net) mod that lets you copy and paste using only the mouse. Hold Ctrl and click once to copy the selected text, or click twice quickly to paste. The trigger button (right or middle), modifier key (Ctrl or Shift), double click timing and an optional beep sound can all be changed from the mod settings.

## How it works

| Action | Result |
| --- | --- |
| Hold **Ctrl** + click **once** | Copy selected text (Ctrl+C) |
| Hold **Ctrl** + click **twice quickly** | Paste (Ctrl+V) |

By default the trigger is the **right mouse button**. A normal right click (without Ctrl) works as usual.

## Installation

1. Install [Windhawk](https://windhawk.net).
2. In Windhawk, click **Create a New Mod** and then **Begin coding**.
3. Delete the template code and paste the contents of `ctrl-click-copy-paste.wh.cpp`.
4. Press **Ctrl+B** to compile, then enable the mod.

## Settings

| Setting | Options | Default |
| --- | --- | --- |
| Trigger mouse button | Right / Middle | Right |
| Modifier key | Ctrl / Shift | Ctrl |
| Double click window (ms) | 100 - 1000 | 350 |
| Beep on copy / paste | On / Off | Off |

## Notes

- Single click copy is delayed by the double click window (350 ms by default), because the mod must wait to see whether a second click follows. Lower the value if copy feels slow, raise it if a double click sometimes copies instead of pasting.
- With Ctrl + right click, the context menu does not open, since the click is consumed by the mod.
- Does not work inside apps running as Administrator, because Windows blocks input from a normal process into an elevated one.
- Some apps that do not support Ctrl+C / Ctrl+V will not react.
- Paste happens at the current cursor position. If text is selected there, it will be replaced.

## Author

[chmuzammil904-a11y](https://github.com/chmuzammil904-a11y)

## License

MIT
