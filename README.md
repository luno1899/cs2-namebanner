# NameBanner

Metamod:Source-плагин для CS2: банит игроков, которые слишком часто меняют ник. Типичный признак обхода бана и спама именами.

По умолчанию: **10 смен ника за 20 секунд — перманентный бан**. Бан записывается в MySQL-таблицу плагина [Admin System](https://github.com/Pisex/cs2-admin_system), никаких RCON-команд плагин не отправляет.

## Как это работает

1. События игроков (подключение, смена настроек, отключение) NameBanner получает от плагина [Utils](https://github.com/Pisex/cs2-menus) через `IPlayerListener`. Своих оффсетов и сигнатур у плагина нет.
2. При каждой смене настроек клиента плагин читает его текущий ник у движка и сравнивает с предыдущим. Если ник изменился, момент смены кладётся в скользящее окно.
3. Когда в окне набралось нужное число смен, вызывается `IAdminApi::AddPlayerPunishment(..., RT_BAN, ...)`. Сам бан и запись в базу делает Admin System.
4. Боты и fake-клиенты игнорируются. Игрок банится один раз за подключение.

Плагин работает только на событиях игрока, постоянных таймеров и обработчиков тиков у него нет, поэтому нагрузка на сервер ничтожна.

NameBanner не проверяет иммунитет и не делает исключений для админов: бан передаётся в Admin System как есть.

## Требования

| Что | Зачем |
|---|---|
| Linux x86_64 сервер CS2 | релиз собирается только под Linux (Steam Runtime) |
| Metamod:Source 2.0.0-dev+1411 (plugin API 17:14) | сборка закреплена на этой версии, на Metamod с API 18 не загружать |
| [Utils](https://github.com/Pisex/cs2-menus/releases) **1.9.1 или новее** | события игроков (`IPlayerListener`); на старых версиях Utils плагин работать не будет |
| [Admin System](https://github.com/Pisex/cs2-admin_system/releases) + [sql_mm](https://github.com/zer0k-z/sql_mm/releases) и настроенная MySQL | хранение банов |

Если Utils или Admin System не загружены, плагин напишет об этом в консоль красным и выгрузит сам себя.

## Установка

1. Скачайте `namebanner-linux-x64.zip` из [Releases](https://github.com/luno1899/cs2-namebanner/releases).
2. Распакуйте архив в `game/csgo/` сервера. Он положит файлы так:
   ```
   addons/namebanner/namebanner.so
   addons/metamod/namebanner.vdf
   addons/configs/NameBanner/settings.ini
   ```
3. Перезапустите сервер или выполните `meta load addons/namebanner/namebanner`.
4. Проверьте `meta list`: должна быть строка `NameBanner (1.0.1) by luno`. В консоли при старте будет `[NameBanner] 1.0.1 loaded` и `Listener registered`.

## Настройка

Файл `addons/configs/NameBanner/settings.ini` (формат KeyValues). Изменения применяются при следующей загрузке плагина.

| Ключ | По умолчанию | Описание |
|---|---|---|
| `threshold` | `10` | сколько смен ника в окне вызывают бан |
| `window_seconds` | `20.0` | размер скользящего окна в секундах |
| `ban_minutes` | `0` | длительность бана в минутах, `0` — навсегда |
| `reason` | `NameBanner` | причина бана; `{changes}` заменяется числом смен |
| `debug` | `0` | `1` — писать в консоль события игроков |

Если файла нет, используются значения по умолчанию.

## Сборка

Нужны Docker и Python 3:

```bash
./build-linux.sh
```

Скрипт подтянет Metamod:Source, HL2SDK и AMBuild (`bootstrap.sh`) и соберёт плагин в Steam Runtime. Готовый пакет окажется в `build-linux-release/package/`. Те же шаги выполняют GitHub Actions на каждый push, а релиз создаётся по тегу `v*`.

## Лицензия

AGPL-3.0, см. [LICENSE](LICENSE). Заголовки `src/include/admin.h` и `src/include/players.h` взяты из проектов [cs2-admin_system](https://github.com/Pisex/cs2-admin_system) и [cs2-menus](https://github.com/Pisex/cs2-menus) (Pisex) и описывают их интерфейсы.

---

## English

NameBanner is a Metamod:Source plugin for CS2 that bans players who change their name too often (default: 10 changes in 20 seconds, permanent ban). It receives player events from the [Utils](https://github.com/Pisex/cs2-menus) plugin (1.9.1+) and stores bans through [Admin System](https://github.com/Pisex/cs2-admin_system)'s MySQL table, so both plugins (plus sql_mm and a configured database) are required. Linux x86_64 only, built against Metamod:Source 2.0.0-dev+1411 (API 17:14). Unzip the release into `game/csgo/` and edit `addons/configs/NameBanner/settings.ini`.
