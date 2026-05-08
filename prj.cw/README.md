# Конвертер обратной развертки 3D

Инструмент для построения 3D-меша из:

- цветного изображения (PNG)
- TIFF файла с XYZ координатами

## Возможности:

- Интерактивный выбор области (ROI)
- Построение 3D mesh
- Визуализация через libigl
- Экспорт в OBJ
- Контроль детализации

---

## Зависимости:

- OpenCV
- Eigen
- libigl
- OpenMP

---

## Сборка:

```bash
mkdir build
cd build
cmake ..
make
```

---

## Сборка (Windows + vcpkg):

```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release