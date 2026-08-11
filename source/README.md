# MerePC Development Notes

`merepc.c` is the primary source file.  It configures the X11 window environment and initializes resources like windows and fonts.

Other `.c` files are the "apps", activities available to the user.  `menu.c` is the starting activity, and users can press ESC at any time to return to this menu.  All other activities are accessible from the menu activity.

Each app defines a `AppStaticType` entity.  When the app is launched, the base system initializes an `AppType` structure and passes it to the app's `->Init()` function.  The `AppType` structure is passed to all other function calls, and contains information about the display and window.  It also contains a place for a pointer to a custom structure that can hold data specific to the application.

Each application is responsible for freeing any resources it has instantiated.  The app's `->Destroy()` function should clean up any allocated resources.

The application's `->EventHandler()` function is called on keyboard and mouse interaction.

The `->Tick()` function is called whenever there are no events waiting to be handled.

The base system loads three fonts that can be used by applications, accessible from the `AppType` data structure passed to each function call.

New apps should create their own `.c` file, as well as add a declaration to the `applications.h` file for the app's global `AppStaticType` structure.
