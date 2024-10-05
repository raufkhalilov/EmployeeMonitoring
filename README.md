# Мониторинг Работы Сотрудников

Этот проект представляет собой клиент-серверное приложение, которое отслеживает время бездействия пользователя и может захватывать скриншоты по запросу.

## Описание

- **Сервер** ожидает подключения клиента и может запрашивать скриншоты или получать информацию о времени бездействия.
- **Клиент** отслеживает время бездействия и может делать скриншоты по команде от сервера.

## Функциональные Возможности

### Мониторинг по Умолчанию

- Если клиент работает без захвата скриншота, он автоматически отслеживает время бездействия и отправляет его на сервер.

### Режим Скриншота

- Если сервер запрашивает скриншот, клиент захватывает экран и сохраняет его как `screenshot.png`.
- Временный скриншот сохраняется как `temp_screenshot.png` на стороне клиента, но он не предназначен для открытия или использования. Вместо этого сервер получает `screenshot.png`.

### Меню Сервера

После подключения клиента сервер предоставляет следующее меню для взаимодействия:

1. **Запрос Скриншота**: Отправляет команду клиенту для захвата текущего экрана. Клиент захватывает скриншот и отправляет его на сервер.
2. **Выход**: Закрывает соединение с текущим клиентом и завершает сеанс.

## Инструкции по Использованию

### Клонирование Репозитория

Для начала клонируйте репозиторий с проектом на ваш локальный компьютер:

```bash
git clone https://github.com/raufkhalilov/EmployeeMonitoring.git
cd EmployeeMonitoring
```

### Сервер

1. Скомпилируйте и запустите серверный код:
   ```bash
   g++ server.cpp -o server -lgdi32 -lgdiplus -lws2_32
   ./server
   ```

2. Сервер будет слушать входящие подключения клиентов.
3. Используйте меню сервера для выбора действий:
   - **Запрос Скриншота**: Отправляет команду клиенту для захвата текущего экрана.
   - **Выход**: Закрывает соединение с клиентом.

### Клиент

1. Скомпилируйте и запустите клиентский код:
   ```bash
   g++ -o client.exe client.cpp -lws2_32 -lgdi32 -lgdiplus -lole32
   ./client
   ```

2. Клиент подключится к серверу.
3. Клиент начнет немедленно отправлять информацию о времени бездействия.
4. Когда сервер запросит скриншот, он будет захвачен и отправлен.

## Структура Кода

### Серверный Код

- **handleClient(SOCKET clientSocket)**: Управляет общением с подключенным клиентом, получает команды и обрабатывает их.
- **menu(SOCKET clientSocket)**: Отображает меню сервера для взаимодействия с пользователем.
- **main()**: Инициализирует сервер, слушает клиентов и создает поток для каждого клиента.

### Клиентский Код

- **getIdleTime()**: Вычисляет время бездействия пользователя.
- **captureScreen(SOCKET connectSocket)**: Захватывает экран и отправляет скриншот на сервер.
- **sendIdleTime(SOCKET connectSocket)**: Отправляет время бездействия на сервер каждые 5 секунд.
- **main()**: Подключается к серверу и запускает потоки для отслеживания времени бездействия и получения команд.

## Требования

- **ОС Windows**: Приложение предназначено для работы в Windows из-за использования специфичных библиотек.
- **GDI+**: Необходимо для захвата скриншотов.

## Инструкции по Сборке

1. Убедитесь, что у вас установлен совместимый компилятор C++ и IDE (например, Visual Studio).
2. Скомпилируйте серверный и клиентский код отдельно.
3. Запустите сервер, а затем клиент.

