#pragma once

#include <Eigen/Core>
#include <opencv2/opencv.hpp>
#include <string>

/**
 * @file unrolling_3d_converter.hpp
 * @brief Класс для преобразования 2D изображения и 3D TIFF в развёрнутую
 * 3D-модель.
 */

/**
 * @class Unrolling3DConverter
 * @brief Основной класс для построения 3D-меша из пары (PNG + TIFF XYZ).
 *
 * Класс выполняет:
 * - Загрузку цветного изображения (PNG)
 * - Загрузку 3D координат (TIFF, 3 канала)
 * - Выбор области интереса (ROI) через GUI
 * - Построение 3D сетки (mesh)
 * - Визуализацию
 * - Сохранение в OBJ
 *
 * @note TIFF должен содержать 3 канала (X, Y, Z)
 */
class Unrolling3DConverter {
public:
  /**
   * @brief Загрузка входных данных.
   *
   * @param png_path Путь к цветному изображению (PNG)
   * @param tiff_path Путь к TIFF файлу с XYZ координатами
   * @return true если загрузка успешна
   * @return false при ошибке
   *
   * @details
   * Проверяет:
   * - корректность загрузки
   * - совпадение размеров
   * - наличие 3 каналов у TIFF
   */
  bool load(const std::string &png_path, const std::string &tiff_path);

  /**
   * @brief Запуск основного GUI-цикла приложения.
   *
   * Управление:
   * - 1..4 — уровень детализации mesh
   * - B — построение 3D
   * - S — сохранение OBJ
   * - Q — выход
   */
  void run();

private:
  /**
   * @brief Callback обработки мыши (выбор ROI).
   */
  static void mouse_callback(int event, int x, int y, int flags,
                             void *userdata);

  /**
   * @brief Построение 3D mesh по выбранной области.
   *
   * @param roi Область интереса
   * @param[out] V Вершины (Nx3)
   * @param[out] F Треугольники (Mx3)
   * @param[out] C Цвета вершин (Nx3)
   *
   * @details
   * Алгоритм:
   * 1. Дискретизация изображения с шагом mesh_step_
   * 2. Извлечение XYZ координат из TIFF
   * 3. Формирование массива вершин
   * 4. Построение регулярной триангуляции
   *
   * @note Автоматически увеличивает шаг, если слишком много вершин
   */
  void build_mesh(const cv::Rect &roi, Eigen::MatrixXd &V, Eigen::MatrixXi &F,
                  Eigen::MatrixXd &C);

  /**
   * @brief Визуализация 3D mesh.
   *
   * @param V Вершины
   * @param F Треугольники
   * @param C Цвета
   */
  void show_3d(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F,
               const Eigen::MatrixXd &C);

private:
  /// Цветное изображение (PNG)
  cv::Mat color_mat_;

  /// Матрица XYZ координат (TIFF)
  cv::Mat xyz_mat_;

  /// Превью для GUI
  cv::Mat preview_mat_;

  /// Начальная точка выделения
  cv::Point start_preview_;

  /// Текущая точка выделения
  cv::Point current_preview_;

  /// Флаг выделения
  bool is_selecting_ = false;

  /// ROI на превью
  cv::Rect preview_selected_rect_;

  /// ROI в полном разрешении
  cv::Rect selected_full_roi_;

  /// Шаг дискретизации mesh
  int mesh_step_ = 1;

private:
  static constexpr int PREVIEW_SCALE = 4;
  static constexpr size_t MAX_VERTICES = 500000;
};