/**
 * @file main.cpp
 * @brief Точка входа в приложение.
 */

#include "unrolling_3d_converter.hpp"
#include <iostream>
#include <string>

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
  std::string png_path;
  std::string tiff_path;

  if (argc >= 3) {
    png_path = argv[1];
    tiff_path = argv[2];
  } else {
    std::cout << "Введите путь к PNG файлу: ";
    std::getline(std::cin, png_path);

    std::cout << "Введите путь к TIFF файлу: ";
    std::getline(std::cin, tiff_path);
  }

  Unrolling3DConverter converter;

  if (!converter.load(png_path, tiff_path)) {
    std::cerr << "Не удалось загрузить данные. Проверьте пути и наличие файлов "
                 "в data/\n";
    return 1;
  }

  converter.run();

  return 0;
}
