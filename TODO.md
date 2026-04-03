# Mafia Final Release Checklist

## Scope Lock (Feature Freeze)
- [ ] Freeze major feature additions after this checkpoint.
- [ ] Only allow: bug fixes, balance tuning, UX clarity updates, and performance/stability patches.
- [ ] Tag this baseline as `v1.0.0-rc1` in release notes.

## Definition Of Done (v1.0.0)
- [ ] 30 complete matches without crash or soft-lock.
- [ ] No unreadable/overlapping UI text in tested resolutions.
- [ ] Instructions page fully explains objective, flow, roles, and controls.
- [ ] Match loop works cleanly: Main Menu -> Lobby -> Role Reveal -> Night/Day -> Game Over -> Main Menu.
- [ ] Clean build from `test_sfml_gcc.bat` on a fresh environment.

## Gameplay & Balance
- [ ] Run at least 100 AI-only matches and record winners.
- [ ] Keep village/mafia win-rate within a healthy target band (roughly 45%-55% each).
- [ ] Verify Joker win condition is possible but not frequent.
- [ ] Validate each role has meaningful impact and no guaranteed-win pattern.
- [ ] Verify tie handling and elimination outcomes are deterministic and fair.

## QA Matrix (Manual)
- [ ] Main menu buttons: Play / Instructions / Settings / Exit.
- [ ] Instructions screen: readable, complete, Back returns to Home.
- [ ] Settings: all sliders/buttons update behavior and persist correctly.
- [ ] Lobby: start flow and player composition sanity.
- [ ] Night actions: GodFather / Mafia / Doctor / Detective / Silencer all work.
- [ ] Day reveal messaging and state transitions are correct.
- [ ] Discussion chat cadence and AI messaging remain coherent.
- [ ] Voting resolves correctly for majority, tie, and edge cases.
- [ ] Quick menu (`Q`): Resume / Home / Exit all function in-match.
- [ ] `Esc` behavior is correct by phase.
- [ ] `Tab` player panel toggle works consistently.
- [ ] Audio controls: `M`, `[`, `]`, `-`, `=` and settings sliders stay in sync.
- [ ] Fullscreen and resize/letterbox behavior stays centered and stable.
- [ ] Return-to-menu and replay multiple rounds without memory/state corruption.

## UI Regression Sweep (Post-Layout Changes)
- [ ] Test UI at 1280x720, 1600x900, 1920x1080, and small windowed mode.
- [ ] Verify panel/card text never clips at top, sides, or bottom.
- [ ] Verify wrapped text stays readable (line spacing and no awkward truncation).
- [ ] Verify form alignment consistency (labels, inputs, value boxes, +/- controls).
- [ ] Verify section spacing remains in 20-30px rhythm on Settings and Lobby.
- [ ] Verify checkbox labels remain vertically centered and do not overlap.
- [ ] Verify instructions two-column layout remains balanced and scroll bar maps correctly.
- [ ] Verify journal content respects inner padding and bottom bounds during long sessions.
- [ ] Verify Last Will textbox wraps correctly and does not draw beyond the text area.
- [ ] Verify all major screens after one full match cycle: Main Menu, Instructions, Settings, Lobby, Role Reveal, Night, Day, Discussion, Voting, Game Over.

## Stability & Diagnostics
- [ ] Ensure startup exceptions are written to `crash.log`.
- [ ] Verify no unhandled exception exits without a logged reason.
- [ ] Check game state resets cleanly between matches.
- [ ] Validate missing optional files fail gracefully with clear messaging.

## Build & Packaging
- [ ] Build `mafia.exe` with `test_sfml_gcc.bat` (Release Candidate).
- [ ] Package executable + required SFML runtime DLLs.
- [ ] Package required assets/config files.
- [ ] Run packaged build on a clean Windows machine.
- [ ] Add release notes for fixes, balance changes, and known limitations.

## Beta Pass
- [ ] Share RC build with 3-5 external testers.
- [ ] Collect issues under 3 buckets only: crash, confusion, unfairness.
- [ ] Patch top-priority issues and cut `v1.0.0`.

