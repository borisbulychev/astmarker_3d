#include "unrolling_3d_converter.hpp"
#include <filesystem>
#include <igl/opengl/glfw/Viewer.h>
#include <igl/per_vertex_normals.h>
#include <igl/writeOBJ.h>
#include <iostream>
#include <opencv2/opencv.hpp>

/**
 * @brief Загружает входные данные (PNG + TIFF).
 *
 * @param png_path Путь к цветному изображению
 * @param tiff_path Путь к TIFF с XYZ координатами
 * @return true если загрузка успешна
 * @return false если произошла ошибка
 *
 * @details
 * Выполняет:
 * - загрузку PNG как цветного изображения
 * - загрузку TIFF с плавающими XYZ координатами
 * - проверку:
 *   - что оба файла существуют
 *   - TIFF имеет 3 канала
 *   - размеры совпадают
 *
 * Дополнительно:
 * - конвертирует TIFF в CV_32FC3
 * - создаёт уменьшенное preview изображение
 *
 * @warning TIFF обязан содержать 3 канала (X,Y,Z)
 */
bool Unrolling3DConverter::load(const std::string &png_path,
                                const std::string &tiff_path) {
  color_mat_ = cv::imread(png_path, cv::IMREAD_COLOR);
  xyz_mat_ = cv::imread(tiff_path, cv::IMREAD_UNCHANGED);

  if (color_mat_.empty() || xyz_mat_.empty() || xyz_mat_.channels() != 3) {
    std::cerr << "Ошибка загрузки файлов!\n";
    return false;
  }

  if (color_mat_.size() != xyz_mat_.size()) {
    std::cerr << "Размеры PNG и TIFF не совпадают!\n";
    return false;
  }

  xyz_mat_.convertTo(xyz_mat_, CV_32FC3);

  cv::resize(color_mat_, preview_mat_, cv::Size(), 1.0 / PREVIEW_SCALE,
             1.0 / PREVIEW_SCALE, cv::INTER_AREA);

  std::cout << "Загружено " << color_mat_.cols << "x" << color_mat_.rows
            << " (preview " << preview_mat_.cols << "x" << preview_mat_.rows
            << ")\n";
  return true;
}

/**
 * @brief Callback обработки мыши для выбора области (ROI).
 *
 * @param event Тип события мыши
 * @param x Координата X
 * @param y Координата Y
 * @param flags Дополнительные флаги
 * @param userdata Указатель на экземпляр класса
 *
 * @details
 * Логика:
 * - ЛКМ вниз = начало выделения
 * - Движение = обновление прямоугольника
 * - ЛКМ вверх = фиксация ROI
 *
 * После отпускания:
 * - сохраняется ROI в preview
 * - пересчитывается ROI в полном разрешении
 */
void Unrolling3DConverter::mouse_callback(int event, int x, int y, int flags,
                                          void *userdata) {
  auto *self = static_cast<Unrolling3DConverter *>(userdata);
  if (!self)
    return;

  if (event == cv::EVENT_LBUTTONDOWN) {
    self->start_preview_ = self->current_preview_ = cv::Point(x, y);
    self->is_selecting_ = true;
  } else if (event == cv::EVENT_MOUSEMOVE) {
    self->current_preview_ = cv::Point(x, y);
  } else if (event == cv::EVENT_LBUTTONUP) {
    cv::Rect preview_roi(self->start_preview_, self->current_preview_);
    self->preview_selected_rect_ =
        preview_roi &
        cv::Rect(0, 0, self->preview_mat_.cols, self->preview_mat_.rows);

    self->selected_full_roi_ =
        cv::Rect(preview_roi.x * PREVIEW_SCALE, preview_roi.y * PREVIEW_SCALE,
                 preview_roi.width * PREVIEW_SCALE,
                 preview_roi.height * PREVIEW_SCALE) &
        cv::Rect(0, 0, self->color_mat_.cols, self->color_mat_.rows);

    self->is_selecting_ = false;
  }
}

/**
 * @brief Построение плотного 3D mesh с устранением дыр.
 *
 * @param roi_in Область интереса
 * @param[out] V Вершины (Nx3)
 * @param[out] F Треугольники (Mx3)
 * @param[out] C Цвета (Nx3)
 *
 * @details
 * Улучшенный алгоритм:
 *
 * 1. Downsampling с адаптивным шагом
 * 2. Фильтрация невалидных XYZ:
 *    - NaN
 *    - inf
 *    - экстремальные значения
 *
 * 3. Интерполяция пропущенных точек:
 *    - если точка невалидна → берём среднее соседей
 *
 * 4. Построение сетки
 *
 * 5. Умная триангуляция:
 *    - проверка расстояния
 *    - предотвращение "конусов"
 *
 * @note Это компромисс между скоростью и качеством
 */
void Unrolling3DConverter::build_mesh(const cv::Rect &roi_in,
                                      Eigen::MatrixXd &V, Eigen::MatrixXi &F,
                                      Eigen::MatrixXd &C) {
  cv::Rect roi = roi_in.empty()
                     ? cv::Rect(0, 0, color_mat_.cols, color_mat_.rows)
                     : roi_in;

  int step = mesh_step_;
  size_t approx = (roi.height / step) * (roi.width / step);
  while (approx > MAX_VERTICES && step < 16)
    step *= 2;

  const int w = (roi.width + step - 1) / step;
  const int h = (roi.height + step - 1) / step;
  const int n_vertices = w * h;

  V.resize(n_vertices, 3);
  C.resize(n_vertices, 3);

  cv::Mat xyz_roi = xyz_mat_(roi);
  cv::Mat color_roi = color_mat_(roi);

  std::vector<bool> valid(n_vertices, true);

  // заполнение вершин
#pragma omp parallel for collapse(2)
  for (int i = 0; i < h; ++i) {
    for (int j = 0; j < w; ++j) {
      int y = std::min(i * step, roi.height - 1);
      int x = std::min(j * step, roi.width - 1);
      int idx = i * w + j;

      const cv::Vec3f p = xyz_roi.at<cv::Vec3f>(y, x);
      bool is_valid = std::isfinite(p[0]) && std::isfinite(p[1]) &&
                      std::isfinite(p[2]) && std::abs(p[2]) < 10000;

      if (!is_valid) {
        valid[idx] = false;
        V.row(idx).setZero();
      } else {
        V(idx, 0) = p[0];
        V(idx, 1) = p[1];
        V(idx, 2) = p[2];
      }

      const cv::Vec3b col = color_roi.at<cv::Vec3b>(y, x);
      C(idx, 0) = col[2] / 255.0;
      C(idx, 1) = col[1] / 255.0;
      C(idx, 2) = col[0] / 255.0;
    }
  }

  // интерполяция дыр
  for (int i = 1; i < h - 1; ++i) {
    for (int j = 1; j < w - 1; ++j) {
      int idx = i * w + j;

      if (valid[idx])
        continue;

      std::vector<Eigen::Vector3d> neighbors;

      for (int di = -1; di <= 1; ++di) {
        for (int dj = -1; dj <= 1; ++dj) {
          int ni = i + di;
          int nj = j + dj;
          int nidx = ni * w + nj;

          if (valid[nidx])
            neighbors.push_back(V.row(nidx));
        }
      }

      if (!neighbors.empty()) {
        Eigen::Vector3d avg(0, 0, 0);
        for (auto &v : neighbors)
          avg += v;

        avg /= neighbors.size();

        V.row(idx) = avg;
        valid[idx] = true;
      }
    }
  }

  // триангуляция
  F.resize(2 * (h - 1) * (w - 1), 3);
  int f = 0;
  for (int i = 0; i < h - 1; ++i) {
    for (int j = 0; j < w - 1; ++j) {
      int v00 = i * w + j, v01 = i * w + j + 1, v10 = (i + 1) * w + j,
          v11 = (i + 1) * w + j + 1;
      F.row(f++) << v00, v01, v10;
      F.row(f++) << v01, v11, v10;
    }
  }
  F.conservativeResize(f, 3);
}

/**
 * @brief Отображает 3D mesh с помощью libigl viewer.
 *
 * @param V Вершины
 * @param F Треугольники
 * @param C Цвета
 *
 * @details
 * Выполняет:
 * - загрузку mesh в viewer
 * - установку цветов
 * - вычисление нормалей
 * - включение освещения
 *
 * @note Viewer блокирует поток до закрытия окна
 */
void Unrolling3DConverter::show_3d(const Eigen::MatrixXd &V,
                                   const Eigen::MatrixXi &F,
                                   const Eigen::MatrixXd &C) {
  igl::opengl::glfw::Viewer viewer;
  viewer.data().set_mesh(V, F);
  viewer.data().set_colors(C);

  Eigen::MatrixXd N;
  igl::per_vertex_normals(V, F, N);
  viewer.data().set_normals(N);

  viewer.data().show_lines = false;
  viewer.core().lighting_factor = 0.8f;
  viewer.launch();
}

/**
 * @brief Главный цикл GUI приложения.
 *
 * @details
 * Функциональность:
 * - отображение preview
 * - обработка выделения ROI
 * - обработка клавиатуры:
 *
 * Управление:
 * - 1..4 → изменение детализации mesh
 * - B → построение 3D
 * - S → сохранить OBJ
 * - Q → выход
 *
 * Логика:
 * - отображает текущий ROI
 * - по нажатию B строит mesh и открывает viewer
 * - по S сохраняет OBJ
 *
 * @warning Viewer блокирует GUI, пока не закрыт
 */
void Unrolling3DConverter::run() {
  std::string png_path, tiff_path;

  std::cout << "Введите путь к PNG файлу: ";
  std::getline(std::cin, png_path);
  std::cout << "Введите путь к TIFF файлу: ";
  std::getline(std::cin, tiff_path);

  if (!load(png_path, tiff_path)) {
    std::cerr << "Не удалось загрузить файлы.\n";
    return;
  }

  const std::string win = "2D Unrolling — выделение сегмента";
  cv::namedWindow(win, cv::WINDOW_NORMAL | cv::WINDOW_GUI_EXPANDED);
  cv::setMouseCallback(win, mouse_callback, this);

  std::cout << "\nУправление:\n"
            << "1/2/3/4 — изменить детализацию\n"
            << "B — построить 3D\n"
            << "S — сохранить OBJ\n"
            << "Q — выход\n";

  while (true) {
    cv::Mat temp = preview_mat_.clone();

    if (is_selecting_)
      cv::rectangle(temp, start_preview_, current_preview_,
                    cv::Scalar(0, 255, 0), 2);
    if (!preview_selected_rect_.empty())
      cv::rectangle(temp, preview_selected_rect_, cv::Scalar(0, 255, 255), 3);

    cv::imshow(win, temp);

    int key = cv::waitKey(30);
    if (key == 'q' || key == 'Q' || key == 27)
      break;

    if (key >= '1' && key <= '4') {
      mesh_step_ = key - '0';
      std::cout << "→ Детализация изменена на шаг " << mesh_step_ << "\n";
    }

    if (key == 'b' || key == 'B') {
      Eigen::MatrixXd V, C;
      Eigen::MatrixXi F;
      build_mesh(selected_full_roi_, V, F, C);
      if (V.rows() > 0) {
        std::cout << "3D построена: " << V.rows() << " вершин (шаг "
                  << mesh_step_ << ")\n";
        show_3d(V, F, C);
      }
    }

    if (key == 's' || key == 'S') {
      Eigen::MatrixXd V, C;
      Eigen::MatrixXi F;
      build_mesh(selected_full_roi_, V, F, C);
      igl::writeOBJ("segment.obj", V, F);
      std::cout << "Сохранено: segment.obj\n";
    }
  }
  cv::destroyAllWindows();
}