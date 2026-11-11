# Сведения о процессе: первое отдельное окно

Статус на 11 ноября 2026 года: нативное окно и optional account/WMI lane
реализованы; полный 14-этапный прогон и отдельные проверки в PowerShell 5.1/7
прошли без SKIP. Точный состав покрытия и оставшиеся ограничения записаны в
`DEVELOPMENT.md`. Ниже сохранён исходный контракт дизайна; будущая настраиваемая
таблица и полная карта продукта остаются отдельной работой.

Дизайн от 10 ноября 2026 года продолжает приоритет процессов и мониторинга из
`docs/ROADMAP.md`. Это проект следующей функции, а не описание готового кода.
Первый результат — отдельное нативное modeless-окно с данными одного процесса.
Настраиваемая полная таблица подробностей, дополнительные массовые колонки,
GPU по процессам и новые изменяющие действия остаются следующими этапами.

## Проверенная основа

Исходники проверены непосредственно перед записью этого документа:

- `view.inc:rowMap`, `RememberSelection` и `actions.inc:QueueAction` получают
  выбранную строку через `LVM_GETNEXTITEM`; для процесса PID хранится по `+3584`,
  FILETIME создания — по `+3600`. Указатель строки нельзя сохранять после
  обработки UI-команды: `ApplySnapshot` меняет front/back buffers.
- `QueueAction` использует общие `actionPid/actionBirth`, блокировку `actionBusy`
  и подтверждения операций. Команда 2010 переключает представление на потоки
  через `inspectedPid/inspectedBirth`. Новое чтение получает собственный маршрут
  до общей ветки Settings и не занимает очередь изменяющих действий.
- `extended.inc:RunExtendedAction` уже открывает свойства файла, папку, дамп и
  affinity; существующая кнопка Details раскрывает **отчёт о действии**.
  Эти назначения сохраняются; новая команда называется «Сведения о процессе».
- `collector.inc:CollectRows` уже читает время, память и I/O, но его
  `memoryBuffer`, `timeCreate/timeKernel/timeUser`, `processIo`, форматирующие
  буферы и `frontBuffer/backBuffer` принадлежат существующему коллектору.
  Отдельный worker имеет собственные буферы и baseline.
- `GuardProcess` запрещает изменяющие действия для ряда процессов. Для чтения
  сведений используются разрешения Win32 и проверка identity; этот guard не
  должен скрывать доступные сведения о текущем, системном или session-0 процессе.
- `taskmanager.asm:simple_loop` пока содержит Translate/DispatchMessage;
  `dpi.inc:UpdateUiDpi` изменяет глобальные `uiDpi/appFont` и основное окно.
  Modeless-окну нужны собственные keyboard routing, DPI и шрифт.

## Окно и данные

Одновременно открыто одно окно, принадлежащее основному HWND. Открытие из меню
или контекстного меню Processes сразу копирует PID, birth и имя выбранной строки
в собственный контекст. Повторное открытие той же identity активирует окно;
открытие другого процесса явно меняет цель и поколение. Сортировка, фильтр,
обновление списка и переход на другую панель не меняют закреплённую цель.
Без выбранной process-строки команда недоступна. При birth=0 окно объясняет,
что identity не подтверждена, и предлагает обновить список и открыть процесс
снова; принятие нового birth только по совпавшему PID запрещено.

В окне доступны Refresh, Close и копирование выделенного текста. Поля read-only,
длинные путь и командная строка имеют прокрутку. Открытие не запускает Explorer,
UAC, shell extensions или изменяющие операции. После первого чтения числовые
показатели обновляются раз в секунду; скрытое/свёрнутое окно приостанавливает
опрос и получает свежий sample при возвращении. Refresh заменяет ожидающий
native-запрос и принудительно запрашивает новую optional-порцию, не накапливая
очередь и не меняя поколение самой цели.

| Поле | Источник и смысл |
| --- | --- |
| Имя, PID, creation identity | Имя из выбранного snapshot; `GetProcessTimes` подтверждает birth через отдельный handle. Время отображается локально с часовым поясом; исходное значение остаётся UTC FILETIME. |
| Полный путь | `QueryFullProcessImageNameW`, Unicode, рост буфера с проверяемым пределом; переполнение даёт явное unavailable. |
| Пользователь и SID | `OpenProcessToken(TOKEN_QUERY)`, `GetTokenInformation(TokenUser)`, `ConvertSidToStringSidW`; `LookupAccountSidW` отдельно дополняет SID именем `DOMAIN\user`. Отказ преобразования имени не скрывает доступный SID. |
| Дескрипторы | `GetProcessHandleCount`; это число открытых дескрипторов, без их перечисления. |
| Память | `GetProcessMemoryInfo(PROCESS_MEMORY_COUNTERS_EX)`: PrivateUsage как private commit, WorkingSetSize отдельно. Working set не называется выделенной/освобождённой памятью. |
| CPU | Kernel/User time из `GetProcessTimes`; процент — только после двух валидных samples одной identity, нормированный на доступную общую CPU capacity. Невалидный интервал сбрасывает baseline. |
| I/O | Все шесть счётчиков `GetProcessIoCounters`; bytes и operations различимы. Скорости read/write/other — разности по монотонному интервалу. Это I/O процесса, включая недисковые устройства, а не физическая активность диска. |
| Командная строка | Необязательный WMI `Win32_Process.CommandLine`, отдельный медленный запрос; пустая валидная строка отличается от null/отказа. |

Базовые данные требуют `PROCESS_QUERY_LIMITED_INFORMATION`; worker сначала
пробует добавить SYNCHRONIZE для достоверного определения завершения, а при
отказе повторяет только limited query.
Для optional memory fallback допустим отдельный handle `QUERY_INFORMATION |
VM_READ`, как в текущем коллекторе; его birth проверяется до использования.
Не добавлять TERMINATE/SET_QUOTA/SET_INFORMATION или включение SeDebugPrivilege
при открытии окна. Уже выбранная пользователем общая privilege policy не меняется.

## Identity, worker и обмен

Контекст содержит PID, birth, состояние закрытия и четыре независимых счётчика:

- `windowCookie` уникален для каждого открытия окна и не переиспользуется.
- `targetGeneration` меняется только при смене PID/birth или закрытии. Poll,
  Refresh и повторная активация той же цели его не меняют.
- `nativeSampleSequence` упорядочивает native samples одной цели. UI принимает
  sample новее последнего применённого, даже если уже ожидает следующий запрос;
  равенство с последним запрошенным sequence не требуется.
- `optionalRequestEpoch` меняется при открытии/смене цели и явном Refresh.
  Секундный native poll его не меняет, optional-запрос не перезапускает и его
  deadline не продлевает. Ответ optional lane сравнивается с её epoch, а не с
  nativeSampleSequence. Refresh инвалидирует старую optional-порцию, но оставляет
  валидным текущий native sample и CPU/I/O baseline той же цели.

Native-worker открывает handle,
сверяет birth до чтения и удерживает его на протяжении sample. Завершение
проверяется нулевым wait на SYNCHRONIZE handle. Без этого права
`GetExitCodeProcess != STILL_ACTIVE` может подтвердить завершение, но значение
259 само по себе не доказывает, что процесс ещё жив: это допустимый код выхода.
Неоднозначное состояние показывается явно. При подтверждённом завершении окно оставляет последний
sample с временем и статусом «Процесс завершён»; PID не переоткрывается как
другая цель. Потеря доступа не выдаётся за подтверждённое завершение.

Сервис details принадлежит приложению: **один global native-worker и один
global optional-worker**, созданные не более одного раза, обслуживают все
последовательные открытия окна. На каждую lane приходятся один активный запрос,
один заменяемый pending request и один заменяемый result slot. Optional lane
обслуживает потенциально медленные account-name lookup и WMI. Если старый вызов
ещё занят, Close/reopen, смена цели и Refresh заменяют pending request; новых
threads не создают. Замена/отмена pending и result освобождает их context refs;
старый активный вызов удерживает только собственный контекст до возврата.
Количество контекстов ограничено этими слотами и текущей UI-ссылкой, а не числом
открытых ранее окон. Каждый request — собственная копия значений, без указателей
на строки основного списка или UI-controls. Native metrics продолжают обновляться
при задержке optional lane.

Результат — неизменяемый snapshot до 192 KiB: путь и command line ограничены
32767 WCHAR с NUL каждый, имя пользователя — 1024, SID — 256; превышение
отмечается ошибкой поля без молчаливого усечения. Snapshot содержит PID/birth,
windowCookie/targetGeneration, sequence либо epoch своей lane, монотонное время
и отдельные value/status/error. Каждая lane атомарно заменяет только свой result
pointer; заменённый непрочитанный блок освобождает публикующий worker.
UI забирает блок, проверяет identity, cookie, targetGeneration и правила
sequence/epoch выше, затем сливает только поля этой lane и освобождает блок.
Native sample не стирает уже принятые optional-поля, а optional completion не
откатывает более свежие native metrics. PostMessage несёт уведомление, а не
владение указателем. Ошибка PostMessage, устаревший cookie, закрытие и повторное
использование HWND имеют однозначный путь освобождения. Новому сообщению READY
выделяется собственная свободная константа после проверки текущих модулей;
существующие 8001h/8002h не переиспользуются.

Close немедленно инвалидирует cookie/targetGeneration, останавливает таймер,
освобождает относящиеся к окну pending/results и закрывает HWND. UI не ждёт
worker и COM. Global workers продолжают существовать; старый активный запрос
видит отмену на ближайшей доступной границе. Его context ref не удерживает
controls, кисти или шрифты уничтоженного окна, а поздний ответ не затрагивает
новое окно. Закрытие основного приложения устанавливает общий stop и отменяет
pending/results обоих global workers. Native handle, token,
события, heap blocks, COM objects/BSTR/VARIANT, кисти и шрифты имеют владельца
и единый путь освобождения. TerminateThread и остановка целевого процесса
для чтения сведений не используются.

## Командная строка и ошибки

Выбран поддерживаемый WMI, работающий асинхронно относительно UI:
`ROOT\CIMV2`, запрос только `ProcessId, CreationDate, CommandLine` с фильтром PID,
`ExecQuery` с RETURN_IMMEDIATELY | FORWARD_ONLY и `Next` с конечным ожиданием.
Worker самостоятельно и сбалансированно инициализирует COM, настраивает proxy
и освобождает его. Результат принимается только при совпадении PID и CreationDate
с native birth после нормализации UTC и точности WMI, с повторной проверкой
живой native identity. Неподтверждённый или запоздавший результат отбрасывается.
Чтение PEB, NtQueryInformationProcess и чужой памяти для command line в этот
этап не входят: они добавляют зависимости от разрядности и внутреннего ABI.

Для optional-порции UI deadline — 3 секунды от создания её epoch, native samples
не ждут его. Истёкший pending request не запускается; UI показывает provider-busy
и следующий явный Refresh создаёт новую epoch. Уже принятые optional-поля той же
цели сохраняются до явного Refresh, смены цели или завершения процесса.
Конечный `Next` не гарантирует отмену всех внутренних COM/lookup-вызовов:
дедлайн ограничивает принятие результата, не является обещанием убить вызов.
Зависшая optional lane не порождает новые threads на каждом Refresh; поле
получает timeout/provider-busy, пока lane не освободится. Для гарантированного
принудительного завершения провайдера позднее потребуется отдельный helper
process; первая версия такой гарантии не заявляет.

Состояния полей: loading, valid (включая 0), unavailable с Win32/HRESULT,
timeout; состояние всей цели отдельно различает identity mismatch, exited
и отсутствие доступа. Ошибка одного источника не стирает успешные поля.
Записи read/CPU/I/O после отказа не продолжают старый interval. Числовой код
ошибки сохраняется сразу после вызова; текст локализуется с fallback на код.
Командная строка, SID и путь не записываются в историю, telemetry или тестовые
логи автоматически; копирование выполняется явным действием пользователя.

## RU/EN, тема, DPI и приёмка

Ресурсы окна и статусы имеют RU/EN версии. Применённый язык определяется
`appLanguage`; нынешний `selectedLanguage` сохраняет настройку для следующего
запуска и сам не перезагружает все strings. Окно следует текущей применённой
локализации. Theme changes применяются отдельно к его native controls через
явное уведомление из ApplyTheme. Per-window DPI/font/layout не изменяют
глобальный uiDpi или основную таблицу. Обработать WM_DPICHANGED, suggested rect,
минимальный размер и длинные значения; Tab/Shift+Tab/Escape, labels и Copy
работают через modeless keyboard routing (`IsDialogMessageW`) основного loop.

Приёмка использует только собственные дочерние процессы, события и временные
файлы: независимые native чтения path/SID/creation/handles/memory/I/O,
контролируемые allocation и I/O, busy→idle CPU, Unicode/spaces и длинную command
line. Проверить отказ прав и mismatch birth без воздействий на рабочие процессы.
Детерминированно проверить замену pending/result, поздний ответ, close/reopen,
HWND reuse, смену цели и навигацию. Optional-ответ через 2.5 секунды должен
применяться после нескольких секундных native polls; Refresh отбрасывает старую
optional epoch, сохраняя валидность текущего native sample. Искусственная
задержка optional backend сохраняет отклик WM_NULL и native updates. При 100
close/reopen с зависшим optional вызовом global workers остаются ровно двумя,
а context refs и handles ограничены числом слотов; проверить их освобождение
после разблокировки вызова и отсутствие утечек.
Hidden UI tests покрывают реальные RU/EN controls, обе темы и DPI 100/150/200%;
они не объявляются ручной desktop/accessibility-проверкой. Реальный WMI тест
на owned child либо проверяет ожидаемую строку, либо явно фиксирует SKIP
optional coverage; отказ не считается успешным чтением. Никаких реальных
UAC, изменений чужих процессов/служб или запуска shell-команд в этих тестах.

Справочные контракты Microsoft: [Win32_Process](https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-process),
[GetProcessHandleCount](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesshandlecount),
[ExecQuery](https://learn.microsoft.com/en-us/windows/win32/api/wbemcli/nf-wbemcli-iwbemservices-execquery),
[IEnumWbemClassObject::Next](https://learn.microsoft.com/en-us/windows/win32/api/wbemcli/nf-wbemcli-ienumwbemclassobject-next).
