cmake_minimum_required(VERSION 3.10)

# Название проекта
project(EmployeeMonitoring)

# Установим стандарт C++
set(CMAKE_CXX_STANDARD 17)

# Поддиректории клиента и сервера
add_subdirectory(client)
add_subdirectory(server)
