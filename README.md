# Windows-CLI-Timer
A timer with a toast notification when it's done.
This functionality is already available in the Windows 10 Alarms & Clock app, but messing with the GUI takes a lot more time and effort. The timer itself is launched as a separate process so you can close the CLI and still receive the notification.
### Usage:
```timer {time in seconds} {optional message}```
Example:
```timer 300 "Cup noodles are ready"```
Note: I renamed the executable to "timer.exe".
