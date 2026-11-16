# UART Audio Recorder

Проект реализует простую Qt/C++ клиентскую часть для записи аудио с STM32 по UART протоколу v2.

Что реализовано:
- Список доступных COM-портов и обновление в UI.
- Кнопка запуска/остановки записи по протоколу START/STOP.
- Корректная сборка кадров команды и проверка CRC32/MPEG-2.
- Потоковый разбор аудиокадров `A5 5A 02 10 ...` с ресинхронизацией.
- Поддержка записи WAV (`PCM16 mono 16000 Hz`) и логирования `packets.log`.
- Оркестратор сессии: каталог `Data/yyyy-MM-dd_HH-mm-ss`, записи, статистика и тайминг.

Ключевые исходные файлы:
- `main.cpp` — точка входа приложения.
- `App/ui/MainWindow.{h,cpp}` — GUI для выбора порта, запуска и отображения статуса.
- `Core` — команда START/STOP, структура кадра.
- `Core` — CRC-32/MPEG-2 с эталонными векторами.
- `Core` — потоковый парсер UART-фреймов.
- `Core` — оркестратор записи, статистика и файловая сессия.
- `Core` — запись WAV с заголовком RIFF/WAVE.
- `Core` — текстовый журнал пакетов.
- `Core` — обёртка над Win32 serial API для текущей среды.

Сборка:

```powershell
cmake -S . -B build -G Ninja
cmake --build build --parallel 4
```

Для запуска в текущем окружении Windows необходимо указать путь к Qt runtime:

```powershell
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.12.0\mingw_64\bin;" + $env:PATH
$env:QT_QPA_PLATFORM = "offscreen"
$env:QT_PLUGIN_PATH = "C:\Users\Pobed\CLionProjects\sound-recording\build\plugins"
.\build\sound_recording.exe
```

Для проверки CRC и протокола:

```powershell
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.12.0\mingw_64\bin;" + $env:PATH
$env:QT_QPA_PLATFORM = "offscreen"
$env:QT_PLUGIN_PATH = "C:\Users\Pobed\CLionProjects\sound-recording\build\plugins"
.\build\core_tests.exe
```

Обратите внимание: в этой среде модуль Qt SerialPort отсутствует, поэтому используется кросс-платформенная Win32-обёртка над COM-портами. Логика протокола и запись WAV сохраняют совместимость с требованиями `Docs/Task.md` и `Docs/UART_PROTOCOL.md`.
