---
tags:
  - command
---

# /track

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/track [ help | off | target | players | refresh | filter ]
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
This command is used to open MQ2Tracking, set tracking parameters/filters, refresh and stop tracking. Options are detailed below.
<!--cmd-desc-end-->

## Options

`help`
:   Show command help.

`off`
:   Stop tracking the current spawn.

`target`
:   Start tracking the current target.

`players [ all | pc | group | npc ]`
:   What you want to track

`refresh [ on | off | # ]`
:   Explicitly turns 'on' or 'off' window refresh, or can be used to set the refresh time in seconds. If no arguments provided, toggles the window refresh on or off

`filter [off | <spawn search string>]`
:   Sets or removes custom [spawn search](../macroquest/reference/general/spawn-search.md) parameters (same as /who)
