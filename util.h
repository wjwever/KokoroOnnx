#pragma once
#include <iostream>
#include <string>
#include <vector>

#include <chrono>
#include <iomanip>
#include <iostream>

inline std::string getCurrentTime() {
  // 获取当前时间点
  auto now = std::chrono::system_clock::now();
  // 转换为time_t，以便使用put_time
  auto now_c = std::chrono::system_clock::to_time_t(now);
  // 转换为time_point，以便获取毫秒部分
  auto duration = now.time_since_epoch();
  auto millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

  // 使用stringstream来构建完整的日期时间字符串
  std::stringstream ss;
  ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
  ss << "." << std::setw(3) << std::setfill('0') << (millis % 1000); // 毫秒部分

  return ss.str();
}

class Timer {
public:
  Timer(const std::string &name = "")
      : name(name), start(std::chrono::high_resolution_clock::now()) {}

  ~Timer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "cost: " << name << " " << duration.count() << " 毫秒";
  }

private:
  std::string name;
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
};
