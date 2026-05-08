# 3D Unrolling Converter

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
