# RLTool

[![Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/chris_tvyt)

RLTool is a C++ game-modification and debugging framework for Rocket League. It is built as a Windows DLL and combines a game-specific Unreal Engine SDK with event hooks, an in-game ImGui interface, and reusable components for managing tools and settings.

## Features

- **Unreal Engine SDK:** generated game-specific classes, structures, parameters, and definitions for working with Rocket League objects and functions.
- **Event hooks:** attach and detach hooks for Unreal `ProcessEvent`, `ProcessInternal`, and `CallFunction`; register pre- and post-event callbacks and manage event allow/block lists.
- **Function logging:** inspect function calls and their associated objects and properties.
- **Object explorer:** search game objects and inspect class information, fields, and functions.
- **In-game interface:** ImGui-based overlay rendered through a Direct3D 11 hook.
- **Components and modules:** organize functionality into lifecycle-managed components and game-state-aware modules.
- **Settings management:** create typed settings with defaults, optional ranges, validation, callbacks, and reset support. Supported types include booleans, integers, floats, strings, colors, rotations, and 2D/3D vectors.
- **Commands and queues:** register commands, execute Unreal console commands, and queue commands for delayed or thread-specific execution.
- **Game-state support:** track states such as the main menu and matches so modules can limit when they run. The state-detection component is game-specific and intended to be adapted as needed.
- **Logging and notifications:** colored console output, optional timestamped file logging, and in-game notifications.
- **Utility extensions:** helper functionality for memory, Unreal memory, formatting, colors, math, and exception handling.

## Build

RLTool is configured as a Visual Studio C++ DLL project. Build it on Windows with Visual Studio 2022 (MSVC v143) using the included solution:

1. Open [`RLTool.sln`](RLTool.sln) in Visual Studio.
2. Select a configuration (`Debug` or `Release`) and platform (`x64` or `Win32`).
3. Build the solution.

The project uses C++20 and includes its listed dependencies in the repository. The generated DLL is intended to run in the target game process; use responsibly and respect the game's terms and applicable rules.

## AI disclosure

RLTool was initially written by hand, but the codebase had become messy. I have used AI over time to iterate on the project and help clean up and improve the source. AI assistance does not change the project's license or ownership.

## License

RLTool is distributed under the [Apache License 2.0](LICENSE).

## Support

If you find RLTool useful, you can support its development on [Ko-fi](https://ko-fi.com/chris_tvyt).
