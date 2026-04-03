# MafiaGame

Mafia social-deduction game written in C++ with SFML.

## Build (Windows)

Use the provided batch script:

```bat
test_sfml_gcc.bat
```

Expected output on success:

```text
Build succeeded: mafia.exe
```

## Run

```bat
mafia.exe
```

## Balance Simulation Mode

Run headless AI simulations to estimate win rates:

```bat
mafia.exe --simulate --matches 100
```

Useful options:

- `--players 5..10`
- `--det on|off`
- `--doc on|off`
- `--joker on|off`
- `--godfather on|off`
- `--silencer on|off`
- `--max-rounds 10..200`
- `--seed <number>`

Example:

```bat
mafia.exe --simulate --matches 300 --players 8 --joker on --godfather on --silencer on
```

## Controls

- Mouse: interact with buttons and choose targets.
- `Esc`: back to Home from non-match pages, quick menu behavior during match.
- `Q`: quick menu in-match (Resume / Home / Exit).
- `Tab`: toggle player hint/info panel in-match.
- `M`: mute/unmute audio.
- `[` / `]`: lower/raise music volume.
- `-` / `=`: lower/raise SFX volume.

## Custom Audio Pack (Premium Feel)

You can replace the generated fallback sounds with your own files.

1. Create either `audio_pack/` or `audio/` in the game folder.
2. Add your files using any of these names (extensions: `.ogg`, `.wav`, `.flac`):

- Menu ambience: `ambient_menu` / `menu_loop` / `menu_ambient`
- In-game ambience: `ambient_game` / `game_loop` / `match_ambient`
- UI click: `ui_click` / `click` / `button_click`
- UI confirm: `ui_confirm` / `confirm` / `button_confirm`
- Chat open/send: `chat_open` / `ui_chat_open` / `chat_notification`
- Vote reveal tick: `vote_tick` / `voting_tick` / `vote_reveal`
- Kill/death: `kill` / `kill_stab` / `death_event`
- Phase transition: `phase_shift` / `phase_transition` / `round_transition`

If a file is missing, the game falls back to generated audio so it always runs.

Tip: In Settings, use the Audio Preview panel to test every event sound plus quick menu/game ambience previews before starting a match.

## Gameplay Flow

1. Main Menu
2. Lobby
3. Role Reveal
4. Night actions
5. Day reveal
6. Discussion
7. Voting
8. Repeat until win condition is reached

## Win Conditions

- Village wins when all mafia-aligned players are eliminated.
- Mafia wins when mafia count is equal to or greater than non-mafia count.
- Joker wins if voted out during daytime voting.

## Final Release Quality Gate

Before shipping `v1.0.0`, ensure:

- No crash/soft-lock during 30 complete matches.
- Instructions screen is complete and readable.
- UI layout remains synchronized and centered in fullscreen/resized windows.
- Audio controls and settings are consistent and persistent.
- Fresh-machine build and run are verified.

## Notes

- This project began as an Advanced Programming course assignment.
- Original wiki: https://github.com/ghminaei/MafiaGame/wiki
