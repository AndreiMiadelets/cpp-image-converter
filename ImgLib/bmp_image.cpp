#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>
#include <iostream>

using namespace std;

namespace {
  /* В BMP отступ равен ширине, умноженной на три и округлённой вверх до числа,
   * делящегося на четыре. */
  int GetBMPStride(int w) {
    constexpr unsigned kBMPOffset = 3;
    constexpr unsigned kBMPmultiple = 4;
    constexpr unsigned kLeftSideOffset = kBMPOffset;
    constexpr unsigned kRightSideOffset = kBMPOffset;
    return kBMPmultiple * ((w * kLeftSideOffset + kRightSideOffset) / kBMPmultiple);
  }
}

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapInfoHeader {
  BitmapInfoHeader() = default;

  BitmapInfoHeader(const Image &image) {
    header_size = sizeof(BitmapInfoHeader);
    width = image.GetWidth();
    height = image.GetHeight();
    planes = 1;
    bit_per_pixel = 24;
    compress = 0;
    image_size = GetBMPStride(image.GetWidth()) * image.GetHeight();
    x_resolution = 11811; // ~300 DPI
    y_resolution = 11811; // ~300 DPI
    count_used_colors = 0;
    count_important_colors = 0x1000000;
  }

  uint32_t header_size;
  int32_t width;
  int32_t height;
  uint16_t planes;
  uint16_t bit_per_pixel;
  uint32_t compress;
  uint32_t image_size;
  int32_t x_resolution;
  int32_t y_resolution;
  int32_t count_used_colors;
  int32_t count_important_colors;
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapFileHeader {
  BitmapFileHeader() = default;

  BitmapFileHeader(const Image &image) {
    signature = {static_cast<std::byte>('B'), static_cast<std::byte>('M')};
    file_size = GetBMPStride(image.GetWidth()) * image.GetHeight()
        + sizeof(BitmapInfoHeader) + sizeof(BitmapFileHeader);
    reserve1 = 0;
    reserve2 = 0;
    offset = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);
  }

  std::array<std::byte, 2> signature{};
  uint32_t file_size;
  uint16_t reserve1;
  uint16_t reserve2;
  uint32_t offset;
}
PACKED_STRUCT_END

ifstream &operator>>(ifstream &ifs, BitmapFileHeader &header) {
  ifs.read(reinterpret_cast<char *>(&header), sizeof(BitmapFileHeader));
  return ifs;
}

ifstream &operator>>(ifstream &ifs, BitmapInfoHeader &info) {
  ifs.read(reinterpret_cast<char *>(&info), sizeof(BitmapInfoHeader));
  return ifs;
}

ofstream &operator<<(ofstream &ofs, const BitmapFileHeader &header) {
  ofs.write(reinterpret_cast<const char *>(&header), sizeof(BitmapFileHeader));
  return ofs;
}

ofstream &operator<<(ofstream &ofs, const BitmapInfoHeader &info) {
  ofs.write(reinterpret_cast<const char *>(&info), sizeof(BitmapInfoHeader));
  return ofs;
}

// напишите эту функцию
bool SaveBMP(const Path &file, const Image &image) {
  BitmapFileHeader header(image);
  BitmapInfoHeader info(image);

  ofstream out(file, ios::binary);
  out << header << info;

  const int h = image.GetHeight();
  const int w = image.GetWidth();
  const int w_strided = GetBMPStride(w);

  for (int y = h - 1; y >= 0; --y) {
    std::vector<char> buff(w_strided, 0);
    const Color *line = image.GetLine(y);
    for (int x = 0; x < w; ++x) {
      buff[x * 3 + 0] = static_cast<char>(line[x].b);
      buff[x * 3 + 1] = static_cast<char>(line[x].g);
      buff[x * 3 + 2] = static_cast<char>(line[x].r);
    }
    out.write(buff.data(), w_strided);
  }

  return out.good();
}

bool CheckLoadImageOnBGR24(const BitmapFileHeader &header, const BitmapInfoHeader &info) {
  // Проверяем сигнатуру
  if (header.signature.at(0) != static_cast<std::byte>('B') && header.signature.at(1) != static_cast<std::byte>('M')) {
    return false;
  }

  // Количество плоскостей
  if (info.planes != 1) {
    return false;
  }

  // Количество бит на пиксель
  if (info.bit_per_pixel != 24) {
    return false;
  }

  // Тип сжатия
  if (info.compress != 0) {
    return false;
  }



  // Разрешение 11811 (300 DPI)
  if (info.x_resolution != 11811 || info.y_resolution != 11811) {
    return false;
  }

  // Количество ключевых цветов
  if (info.count_important_colors != 0x1000000) {
    return false;
  }

  return true;
}

// напишите эту функцию
Image LoadBMP(const Path &file) {
  BitmapFileHeader header{};
  BitmapInfoHeader info{};

  ifstream ifs(file, ios::binary);
  ifs >> header >> info;
  if (ifs.bad()) {
    std::cout << "I/O error while reading\n";
    return {};
  }


  if (!CheckLoadImageOnBGR24(header, info)) {
    return {};
  }

  int w = info.width;
  int h = info.height;

  const int w_strided = GetBMPStride(w);
  Image result(w, h, Color::Black());

  for (int y = h - 1; y >= 0; --y) {
    std::vector<char> buff(w_strided);
    ifs.read(buff.data(), w_strided);
    if (ifs.bad()) {
      std::cout << "I/O error while reading\n";
      return {};
    }

    Color *line = result.GetLine(y);
    for (int x = 0; x < w; ++x) {
      line[x].b = static_cast<byte>(buff[x * 3 + 0]);
      line[x].g = static_cast<byte>(buff[x * 3 + 1]);
      line[x].r = static_cast<byte>(buff[x * 3 + 2]);
    }
  }

  return result;
}

}  // namespace img_lib
