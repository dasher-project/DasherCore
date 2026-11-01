# Custom Actions & Control Mode

## Overview

DasherCore has a **unified action system**. Both alphabet symbol nodes (typing a letter) and control nodes (editing commands like delete, move, speak) use the same `ControlAction` base class and `ActionRegistry`. Frontends can register **arbitrary custom actions** via the C API — anything from sending an API call to opening another app.

**Key files:**
- `src/DasherCore/ControlManager.h` — Action classes, registry, control node tree
- `src/DasherCore/ControlManager.cpp` — Implementation + XML parser
- `Data/control/control.xml` — Default control command tree
- `src/dasher.h` — C API (`dasher_register_action`)
- `src/CAPI.cpp` — C API implementation

## How It Works

```
control.xml  ─┐                    ┌─→ Built-in action (StopAction, MoveAction, ...)
               ├─→ ActionRegistry ──┤
alphabet XML ─┘                    └─→ Custom action (your callback via C API)

Node entered (Do())
    └─→ ControlAction::execute(interface)
            ├─→ Built-in: calls interface methods (ctrlMove, Speak, etc.)
            └─→ Custom: calls your C callback with action name + XML attributes
```

### Lifecycle

1. Frontend calls `dasher_register_action()` to register custom action types
2. `dasher_set_screen_size()` triggers `Realize()`, which creates the `CNodeCreationManager`
3. If `BP_CONTROL_MODE` is on, `CControlManager` is created and parses `control.xml`
4. During parsing, each XML element name is looked up in the `ActionRegistry`
5. When the user navigates into a control node, `CContNode::Do()` executes all the node's actions

### Timing constraint

Custom actions must be registered **before** `dasher_set_screen_size()` for them to be available during initial `control.xml` parsing. If registered after, the action type will be registered in the registry, but already-parsed nodes won't include it until the control box is rebuilt (e.g. by toggling `BP_CONTROL_MODE` off and on).

## Built-in Actions

These are registered automatically by `CControlManager::registerBuiltinActions()`:

| XML name | Attributes | Description |
|---|---|---|
| `stop` | - | Stop Dasher and trigger on-stop behaviour (speak/copy on stop) |
| `pause` | - | Pause Dasher motion |
| `move` | `forward`=`yes`\|`no`, `dist`=`char`\|`word`\|`sentence`\|`paragraph`\|`line`\|`page`\|`all` | Move cursor |
| `delete` | `forward`=`yes`\|`no`, `dist`=(same as move) | Delete text |
| `speak` | `what`=`new`\|`repeat`\|`cancel`\|`all`\|`paragraph`\|`sentence`\|`line`\|`word` | Speak text via TTS callback |
| `copy` | `what`=(same as speak) | Copy text to clipboard |
| `speak_cancel` | - | Cancel ongoing speech |

**Not built-in:** `select` (RFC 0021) — frontend-executed text selection with
attributes `forward`=`yes`\|`no`, `dist`=`char`\|`word`\|`paragraph`\|`all`.
Nodes carrying `<select>` only appear when the frontend has registered a
handler — see [Gating](#gating-rfc-0021) below.

## Gating (RFC 0021)

An action element whose name has **no registered handler** (built-in or
custom) is dropped at parse time, and a node whose only actions were such
elements is **pruned from the tree entirely** — a node that silently does
nothing when zoomed is worse than no node. Consequences for control.xml
authors:

- A node mixing a registered and an unregistered action **degrades**: the
  registered action survives, the node stays.
- The prune cascades: a group whose every template child was pruned goes
  too (an `<alph/>` escape or `<root/>` loop does not keep an emptied group
  alive; neither do mutually-referencing dead groups).
- `ref`s to pruned named nodes are stripped, not rendered.
- One informational message per parse reports the prune:
  `"Control: N node(s) hidden (no registered handler for their action)"`.

Note that **any** element inside a `<node>` that is not `node`/`ref`/
`alph`/`root` and has no registered handler counts as an unregistered
action for gating purposes — including elements a future control.xml
version might introduce as structural. Frontends that want such nodes to
render must register a handler (a no-op handler is fine).


**Alphabet XML compatibility names** (also registered for backward compatibility):

| XML name | Attributes | Maps to |
|---|---|---|
| `stopDasherAction` | — | `StopAction` |
| `pauseDasherAction` | — | `PauseAction` |
| `stopTTSAction` | — | `SpeakCancelAction` |
| `fixedTTSAction` | `text`="..." | `FixedSpeakAction` |

## Custom Actions via C API

### Registration

```c
// Callback signature
typedef void (*dasher_action_callback)(const char* name,
                                       int attr_count,
                                       const char** attr_keys,
                                       const char** attr_values,
                                       void* user_data);

// Register before dasher_set_screen_size()
dasher_register_action(ctx, "my_action", my_callback, user_data);
```

### Callback parameters

- **`name`**: The action element name from XML (e.g. `"my_action"`)
- **`attr_count`**: Number of XML attributes
- **`attr_keys`**: Array of attribute names (e.g. `["endpoint", "method"]`)
- **`attr_values`**: Array of attribute values (e.g. `["/api/save", "POST"]`)
- **`user_data`**: The pointer you passed to `dasher_register_action`

Arrays are only valid during the callback. Copy strings if you need them later.

### Example: API call action

**control.xml:**
```xml
<control>
  <node label="API">
    <send_api endpoint="/api/save" method="POST"/>
    <alph/>
    <root/>
  </node>
</control>
```

**Frontend code (C):**
```c
void on_action(const char* name, int count, const char** keys, const char** vals, void* ud) {
    MyState* state = (MyState*)ud;
    const char* endpoint = NULL, *method = NULL;
    for (int i = 0; i < count; i++) {
        if (strcmp(keys[i], "endpoint") == 0) endpoint = vals[i];
        if (strcmp(keys[i], "method") == 0) method = vals[i];
    }
    if (endpoint) {
        // Make HTTP request, open another app, trigger IoT device, etc.
        printf("Action %s: %s %s\n", name, method ? method : "GET", endpoint);
    }
}

dasher_ctx* ctx = dasher_create(data_dir, user_dir, NULL);
dasher_register_action(ctx, "send_api", on_action, &my_state);
dasher_set_screen_size(ctx, 800, 600);
// Enable control mode
int bp_control_mode = dasher_find_parameter_key("BP_CONTROL_MODE");
dasher_set_bool_parameter(ctx, bp_control_mode, 1);
```

### Example: Swift/Kotlin/etc.

The C API is callable from any language with C FFI. In Swift:

```swift
let cb: @convention(c) (UnsafePointer<CChar>?, Int32,
                        UnsafePointer<UnsafePointer<CChar>?>?,
                        UnsafePointer<UnsafePointer<CChar>?>?,
                        UnsafeMutableRawPointer?) -> Void = { name, count, keys, vals, ud in
    // Handle action
}
dasher_register_action(ctx, "my_action", cb, nil)
```

## control.xml Format

```xml
<?xml version="1.0" encoding="UTF-8"?>
<control name="">
  <!-- Escape: bridge back to alphabet node tree -->
  <alph/>

  <!-- Node with label, colour, children, and actions -->
  <node label="Stop" color="242">
    <stop/>           <!-- action -->
    <alph/>           <!-- child: bridge to alphabet -->
    <root/>           <!-- child: loop back to root -->
  </node>

  <!-- Named node for cross-references -->
  <node name="CTL_MOVE" label="Move" color="-1">
    <alph/>
    <!-- Nested children -->
    <node label="&lt;=" color="-1">
      <node label="Word" color="-1">
        <move forward="no" dist="word"/>
        <alph/>
        <ref name="CTL_MOVE"/>  <!-- reference (allows cycles) -->
      </node>
    </node>
  </node>
</control>
```

### Elements

| Element | Purpose |
|---|---|
| `<control>` | Root element |
| `<node>` | Defines a control node. Attributes: `name` (for refs), `label` (display text), `color` (colour index) |
| `<alph/>` | Bridge back to alphabet node tree (child = `nullptr` in successors) |
| `<root/>` | Loop back to the root control template |
| `<ref name="..."/>` | Reference to a named node (allows cycles) |
| Any registered action name | Creates an action attached to the current node |

### Colour indices

| Index | Colour | Meaning |
|---|---|---|
| `8` | Grey | Neutral/control |
| `240` | Green | Safe/positive actions |
| `241` | Amber | Caution actions |
| `242` | Red | Destructive actions |
| `-1` | Auto | Auto-cycles through palette colours |
| Other | Blue-purple | Default |

## Enabling Control Mode

Control mode is off by default. Enable it via the `BP_CONTROL_MODE` boolean parameter:

```c
int key = dasher_find_parameter_key("BP_CONTROL_MODE");
dasher_set_bool_parameter(ctx, key, 1);
```

When enabled, a control node is grafted as an extra sibling at the root level, taking ~5% of the probability space. The user navigates into it to access editing commands and custom actions.

### Slow control box

`BP_SLOW_CONTROL_BOX` halves the zoom speed when inside control nodes, making precise selection easier:

```c
int slow_key = dasher_find_parameter_key("BP_SLOW_CONTROL_BOX");
dasher_set_bool_parameter(ctx, slow_key, 1);
```

## Testing

Tests are in `tests/test_control_actions.cpp` and run via `ctest` in CI.

To run locally:
```bash
cd build && ctest -R control_actions --output-on-failure
```
