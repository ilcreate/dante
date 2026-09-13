# Приёмка JSON-логов

Дата: 2026-09-13. Требования: [план и контракт](../../plans/logformat-json.md).

## Сверка кода с ТЗ

Проверены изменения всех шести этапов, точки вызова логгера и прямые операции
вывода в сервере. Формат подключён к общему логгеру; поля соединений и итогов
берутся из C-структур. Текст `message` не разбирается для получения полей.

| Требование | Реализация и проверка |
|---|---|
| Глобальный raw/json, default raw, ошибки значений/повторов/размещения | `config_parse.y`, `config_scan.l`, config/lifecycle tests |
| Активация при разборе, SIGHUP и обновление workers | `serverconfig.c`, `sockd_child.c`, `shmemconfig.c`; тест переходов с сохранением PID и работающей TCP-сессии |
| Один объект на строку, обязательные поля и экранирование | `logjson.c`, `log.c`; стандартный JSON-парсер, Unicode, некорректный UTF-8, бинарные и длинные сообщения, ASan/UBSan |
| Ограниченная память, allocation failure, pipe/FIFO, усечение | Два ограниченных буфера, резерв на стеке, PIPE_BUF; проверки отказов и `truncated` |
| События, правила, verdict, адреса и proxy, authentication | `iologevent()`; матрица операций/флагов, native producer tests и реальные CONNECT/BIND/UDP |
| Закрытие, причины, сторона, timeout, длительность и счётчики | `io_sessionlog()`, `io_delete()`, `siginfo()`; известный TCP/UDP-трафик, BIND-направления, UDP buckets и повторные snapshots |
| Сигналы, stack trace, ring buffer, fatal scanner | `signalslog()`, `slogstack()`, `YY_FATAL_ERROR`; signal guards, реальные сигналы, livedebug-сборка |
| Получатели и совместимость raw | Общий `dolog()`, сохранённый отбор; файлы, stdout/stderr, errorlog, перехват границы syslog, сравнение raw fixtures |
| Конкурентная запись | Реальные UDP-сессии в нескольких I/O workers, длинные payload, файл и FIFO; парсится каждая запись |
| Сборка, регрессии, source archive | Сервер и клиентские библиотеки, logging suite, statistics API, smoke; CI запускает проверки из release archive |

Независимый аудит production-кода этапов 1–5 не выявил подтверждённых
критических или существенных дефектов реализации. Он обнаружил гонку в тесте
UDP-снимка без target: ответ UDP ASSOCIATE приходил раньше передачи сессии
I/O worker. Тест теперь сначала наблюдает готовность через ограниченные
повторные запросы снимка; дальнейшая проверка повторного снимка остаётся строгой.

## Выполненные проверки

| Среда | Результат |
|---|---|
| macOS 26.2 arm64, Apple clang 17.0.0 | Свежая default-сборка сервера и static/dynamic client; 102 logging-теста: 100 passed, 2 ожидаемых skip; отдельный client-тест passed; statistics API и relay smoke passed |
| Debian 12 arm64, GCC 12.2, Docker с `--network none` | Свежая default-сборка из архива с нормализованным временем; тот же полный suite и отдельный client-тест passed; statistics API и relay smoke passed |
| macOS и Debian 12, `--disable-client --enable-livedebug` | Новый `ci/build-test-variant.sh` собрал обе версии из source archive; полный server suite, statistics API, smoke и все 4 специальных runtime-теста passed, включая настоящий fatal ring dump |
| Проверки CI-инфраструктуры | 19 Python unit tests passed, включая LF/Unicode JSONL framing; actionlint 1.7.12 и shellcheck 0.9.0 passed |
| Сериализатор под ASan/UBSan | Независимый аудит заново собрал сериализатор: все 18 тестов passed |
| Source distribution | `make distdir` passed; byte-сверка parser/scanner, logjson, logging/stats tests, runner, man page, примеров и JSON-документации |

Два пропуска обычного server suite — client-library test и fatal ring runtime;
они выполнены отдельно в соответствующих сборках. Все 102 уникальных сценария
покрыты этими вариантами. IPv6 network-тесты исполнились на обеих платформах.
Число диагностических строк зависит от расписания workers; проверяется каждая
строка, а фиксированный подсчёт требуется для контролируемых traffic events.

Существующие предупреждения компилятора о старом стиле C остаются. Проверка
man page через mandoc не добавила диагностик относительно этапа 5; старые
неизвестные макросы и предупреждения в других разделах документа сохранены.
Hosted GitHub Actions и сборка/публикация всех пакетов не запускались из этой
сессии: workflow подготовлен для штатного запуска. Локально исполнились
его общий runner и variant-builder на macOS и Linux.

При подготовке Linux-прогона потребовалось сохранить одинаковое время файлов
как у release archive, чтобы make использовал поставляемые autotools-файлы.
Попытка дополнительно убрать все capabilities контейнера лишила uid0 доступа
к тестовому API-сокету владельца nobody; финальный прогон со стандартными
container capabilities и отключённой сетью прошёл. Это ограничения тестового
окружения, а не изменения production-кода.

## Наблюдаемый overhead

Реальный `log.o`, без подмены часов, allocator или write. Одинаковая
последовательность: 50% форматированных диагностик с escaping/UTF-8 и 50%
структурированных TCP session_close с адресами и счётчиками. После прогрева
выполнены 7 пар по 100 000 событий, порядок raw/JSON чередуется; ниже медианы
на macOS arm64 при `-O2`. Запуск процессов и Python не входят в замер.

| Получатель | Raw, wall / 100k | JSON, wall / 100k | JSON/raw wall | JSON/raw CPU |
|---|---:|---:|---:|---:|
| `/dev/null` | 0,054073 с | 0,231865 с | 4,288× | 4,298× |
| Append-файл, без fsync | 0,217378 с | 0,403310 с | 1,855× | 1,855× |

Для `/dev/null` это 0,54 → 2,32 мкс/событие, дополнительно около 1,78 мкс.
Размер записи на контрольных данных в среднем 129,75 → 446,25 байта.
Микробенчмарк измеряет логгер и buffered kernel writes, не пропускную
способность SOCKS и не durable-storage latency. Формат событий, получатель,
конкурентность и платформа меняют результат; порог скорости в CI не задан.
Команды воспроизведения находятся в [test guide](../../../sockd/tests/README.logformat.md).

## Локальная демонстрация с curl

Собранный бинарник и конфигурация оставлены в `/tmp/dante-json-stage6/`:

- `sockd` — macOS arm64 бинарник;
- `sockd.conf` — `logformat: json` первой директивой, SOCKS на `127.0.0.1:54043`;
- `accept.jsonl`, `error.jsonl` — проверенные журналы;
- `verification.json` — команды curl, коды завершения и счётчики записей.

Конфигурация ограничивает доступ локальным клиентом и loopback-назначениями.
Правила включают `log: connect disconnect error`.
`external.log.warning.error: ECONNREFUSED` дополнительно направляет диагностику
отказа соединения в errorlog. `accept.jsonl` — общий журнал, он также содержит
структурированные ошибки и итоги сессий; `errorlog` фильтрует по severity.

Выполнены запросы с явным отключением обхода прокси:

```sh
curl --silent --show-error --max-time 10 --noproxy '' \
  --socks5-hostname 127.0.0.1:54043 http://127.0.0.1:54044/
curl --silent --show-error --max-time 10 --noproxy '' \
  --socks5-hostname 127.0.0.1:54043 http://127.0.0.1:54048/
```

Первый получил HTTP 200 и `Dante JSON proxy acceptance OK` (curl exit 0).
Второй обращался к закрытому порту и получил SOCKS5 connection refused
(curl exit 97). Все 13 строк общего журнала и все 2 строки errorlog разобраны
`json.loads`, проверены версия схемы и тип `truncated`. В общем журнале есть
`accept`, `connect`, `error`, `session_close`; в errorlog — warning об отказе
соединения и alert завершения сервера. Raw-строк в этих файлах нет.

Тестовый HTTP-сервер и прокси остановлены; файлы оставлены для просмотра.
Повторный запуск прокси:

```sh
/tmp/dante-json-stage6/sockd -f /tmp/dante-json-stage6/sockd.conf \
  -p /tmp/dante-json-stage6/sockd.pid
```

Для повторения HTTP-запроса нужен запущенный loopback HTTP target; указанные
выше порты относятся к записанному прогону и после него освобождены.

## Границы результата

Ранние сообщения до чтения директивы остаются raw. При смене формата SIGHUP
допускает переходное смешение. Внешний syslog-префикс, его усечение и ошибки
постоянного хранилища не входят в гарантию JSON Lines. Payload требует `log: data`;
пароли не сериализуются. TCP_INFO остаётся текстом. Сквозной session ID,
новая схема statistics API и структурирование всей произвольной диагностики
не входили в согласованную задачу.

Внешние GSSAPI/PAM/LDAP-сервисы и native hostid backend не поднимались;
username/proxy metadata проверены native drivers. Syslog проверен на границе
вызова API с контролем payload, priority и facility; оформление системной
службой не изменялось и отдельным syslog-сервисом не проверялось.
