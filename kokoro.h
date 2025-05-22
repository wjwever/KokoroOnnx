/*************************************************************************
    > File Name: kokoro.h
    > Author: frank
    > Mail: 1216451203@qq.com
    > Created Time: 2025年05月13日 星期二 14时31分39秒
 ************************************************************************/
#pragma once
#include "cppjieba/Jieba.hpp"
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <onnxruntime_cxx_api.h>
#include <string>
#include <thread>

class Tts {
public:
  Tts(const std::string &kokoro_onnx, const std::string &tokens,
      const std::vector<std::string> &lexicons, const std::string &voice_bin,
      const std::string &jieba_dir);
  void run(const std::string &text);

  Ort::Env env_;
  Ort::SessionOptions session_options_;
  std::unique_ptr<Ort::Session> session_;
  std::unique_ptr<cppjieba::Jieba> _jieba;

  std::vector<const char *> input_names_;
  std::vector<std::vector<int64_t>> input_dims_;
  std::vector<const char *> output_names_;
  std::set<char> _punc_set;

  void setupIO();
  void getCustomMetadataMap(std::map<std::string, std::string> &data);

  int32_t _sample_rate;
  int32_t _max_len;

  std::map<std::string, int32_t> _token2id;
  std::map<std::string, std::vector<std::string>> _word2token;
  std::map<std::string, std::vector<float>> _voices; // voice -> 510 x 1 x 256
  std::vector<int64_t> _style_dims;                  // 510 1 256

  void run(const std::string &text, const std::string &voice, std::vector<float>& out_data);
  void infer(std::vector<int64_t>& tokenids, std::vector<float>& style, float speed, std::vector<float>& out_data);
  std::vector<std::string> split_ch_eng(const std::string &text);

private:
  void load_tokens(const std::string &);
  void load_lexicons(const std::vector<std::string> &);
  int load_voices(const std::vector<std::string> &speaker_names,
                  std::vector<int64_t> &dims, const std::string &voices_bin);
};
