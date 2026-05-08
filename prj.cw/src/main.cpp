/**
 * @file main.cpp
 * @brief Точка входа в приложение.
 */

#include "unrolling_3d_converter.hpp"
#include <iostream>

/**
 * @brief Главная функция.
 *
 * @param argc Количество аргументов
 * @param argv Аргументы командной строки
 *
 * @details
 * Аргументы:
 * - argv[1] — путь к PNG
 * - argv[2] — путь к TIFF
 */
int main(int argc, char **argv) {
  std::string png_path =
      (argc > 1)
          ? argv[1]
          : "~/prj.cw/data/scroll004_fst_img_align_11_2676.png";
  std::string tiff_path =
      (argc > 2)
          ? argv[2]
          : "~/prj.cw/data/scroll004_rec_3d_11_2676.tif";

  Unrolling3DConverter converter;
  if (!converter.load(png_path, tiff_path)) {
    std::cerr << "Не удалось загрузить данные. Проверьте пути и наличие файлов "
                 "в data/\n";
    return 1;
  }

  converter.run();
  return 0;
}
