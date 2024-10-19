#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>
#include <thread>
#include <list>
#include <atomic>

#include "CRC32.hpp"
#include "IO.hpp"

/// @brief Переписывает последние 4 байта значением value
void replaceLastFourBytes(std::vector<char> &data, uint32_t value) {
  std::copy_n(reinterpret_cast<const char *>(&value), 4, data.end() - 4);
}

void check_value_of_x_to_y(size_t of, size_t to,   std::atomic<uint32_t>& four_bytes, std::atomic<size_t>& progress, std::atomic<bool>& finish, const uint32_t& originalCrc32, const uint32_t& original_injetion)
{
  uint32_t resultic = 0; 
  for (size_t i = of; i<to; i++)
  {
    resultic = (uint32_t)i;
    // Вычисляем CRC32 текущего вектора result
    auto currentCrc32 = crc32((const char*)&resultic, 4, original_injetion);

    if (currentCrc32 == originalCrc32) {
      four_bytes = resultic;
      finish = true;
    }
    progress++;
  }
}

/**
 * @brief Формирует новый вектор с тем же CRC32, добавляя в конец оригинального
 * строку injection и дополнительные 4 байта
 * @details При формировании нового вектора последние 4 байта не несут полезной
 * нагрузки и подбираются таким образом, чтобы CRC32 нового и оригинального
 * вектора совпадали
 * @param original оригинальный вектор
 * @param injection произвольная строка, которая будет добавлена после данных
 * оригинального вектора
 * @return новый вектор
 */



std::vector<char> hack(const std::vector<char> &original,
                       const std::string &injection) {
  const uint32_t originalCrc32 = crc32(original.data(), original.size());


  std::vector<char> result(original.size() + injection.size() + 4);
  auto it = std::copy(original.begin(), original.end(), result.begin());
  std::copy(injection.begin(), injection.end(), it);

  const uint32_t original_injetion = crc32(&(*result.begin())+original.size(), injection.size(), originalCrc32);

  const size_t maxVal = std::numeric_limits<uint32_t>::max();

  unsigned int howe_many = std::thread::hardware_concurrency();  
  std::atomic<bool> IsFinish = false; 
  
  size_t s = maxVal / howe_many;
  
  std::atomic<size_t> progress = 0;

  std::atomic<uint32_t> four_bytes;

  std::vector<std::thread> a;
  for (size_t i = 0; i<howe_many; i++)
  {
    //size_t of, size_t to,   std::atomic<uint32_t>four_bytes, std::atomic<size_t>& progress, std::atomic_bool finish, const uint32_t& originalCrc32, const uint32_t& original_injetion)
    std::thread b(check_value_of_x_to_y, (size_t)i*s, (size_t)(i+1)*s, std::ref(four_bytes), std::ref(progress), std::ref(IsFinish), std::cref(originalCrc32), std::cref(original_injetion));
    a.push_back(std::move(b));
  }

  while (!IsFinish) 
  {
    if (progress >= maxVal)
    {
      break;
    }
    if (progress%1000==0)
    {
      //std::cout << (double)((double)progress / (double) maxVal) << std::endl;
    }
  };

  for (size_t i = 0; i<howe_many; i++)
  {
     a[i].detach();
  }

  if (!IsFinish)
  {
    throw std::logic_error("Don't find hack SRC32!!!");
  }


  uint32_t value_r = (uint32_t)four_bytes;

  replaceLastFourBytes(result, four_bytes);

  return result;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Call with two args: " << argv[0]
              << " <input file> <output file>\n";
    return 1;
  }

  try {
    const std::vector<char> data = readFromFile(argv[1]);
    const std::vector<char> badData = hack(data, "He-he-he");

    writeToFile(argv[2], badData);
  } catch (std::exception &ex) {
    std::cerr << ex.what() << '\n';
    return 2;
  }
  return 0;
}
