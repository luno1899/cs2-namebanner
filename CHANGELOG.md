# Changelog

## 1.0.1

- Bans go through Pisex Admin System (`IAdminApi`) into its MySQL table instead of a console command.
- Player events come from the Utils plugin (`IPlayerListener`, Utils 1.9.1+); NameBanner no longer ships its own game data, offsets or SourceHook hooks.
- Config moved to `addons/configs/NameBanner/settings.ini` (KeyValues): `threshold`, `window_seconds`, `ban_minutes`, `reason`, `debug`.
- Defaults: 10 name changes within 20 seconds, permanent ban, reason `NameBanner`.
- Startup is logged to the console (plugin loaded, Utils and Admin System found, listener registered).
- Build is pinned to Metamod:Source `7ec0f16` (2.0.0-dev+1411, plugin API 17:14) and alliedmodders/hl2sdk `cs2` at `84a823d`.
