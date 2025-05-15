/*************************************************************************
    > File Name: kokoro.cpp
    > Author: frank
    > Mail: 1216451203@qq.com
    > Created Time: 2025年05月13日 星期二 14时32分01秒
 ************************************************************************/
#include "kokoro.h"
#include "onnxruntime_cxx_api.h"
#include "util.h"
#include "wave-writer.h"
#include "algorithm"

/*--------------------util------------------*/
std::vector<std::string> split_string(const std::string &s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);

  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }

  return tokens;
}
std::vector<float>  load_vec(const std::string& fin) {
    std::vector<float> ret;
    std::ifstream stream(fin);
    std::string line;
    while(std::getline(stream, line)) {
        auto vec = split_string(line, ' ');      
        for (auto s :vec) {
            ret.push_back(stof(s));
        }
    }
    return ret;
}
std::vector<std::string> utf8_to_charset(const std::string &input) {
    std::vector<std::string> output;
    std::string ch;
    for (size_t i = 0, len = 0; i != input.length(); i += len) {
      unsigned char byte = (unsigned)input[i];
      if (byte >= 0xFC) // lenght 6
        len = 6;
      else if (byte >= 0xF8)
        len = 5;
      else if (byte >= 0xF0)
        len = 4;
      else if (byte >= 0xE0)
        len = 3;
      else if (byte >= 0xC0)
        len = 2;
      else
        len = 1;
      ch = input.substr(i, len);
      output.push_back(ch);
    }
    return output;
  }
/* ----------------------------------------- */

Tts::Tts(const std::string &kokoro_onnx, const std::string &tokens,
         const std::vector<std::string> &lexicons,
         const std::string &voices_bin, const std::string &jieba_dir) {
  env_ = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "kokoro");
  session_options_.SetInterOpNumThreads(1);
  session_ = std::make_unique<Ort::Session>(env_, kokoro_onnx.c_str(),
                                            session_options_);
  load_tokens(tokens);
  load_lexicons(lexicons);

  std::map<std::string, std::string> meta;
  getCustomMetadataMap(meta);

  for (auto str : split_string(meta["style_dim"], ',')) {
    _style_dims.push_back(stoi(str));
  }
  std::vector<std::string> speaker_names =
      split_string(meta["speaker_names"], ',');

  load_voices(speaker_names, _style_dims, voices_bin);
  _sample_rate = 24000;
  _max_len = _style_dims[0] - 1;
  // Please download dict files form
  // https://github.com/csukuangfj/cppjieba/releases/download/sherpa-onnx-2024-04-19/dict.tar.bz2
  std::string kDictPath = jieba_dir + "/jieba.dict.utf8";
  std::string kHmmPath = jieba_dir + "/hmm_model.utf8";
  std::string kUserDictPath = jieba_dir + "/user.dict.utf8";
  std::string kIdfPath = jieba_dir + "/idf.utf8";
  std::string kStopWordPath = jieba_dir + "/stop_words.utf8";
  _jieba = std::make_unique<cppjieba::Jieba>(
      kDictPath.c_str(), kHmmPath.c_str(), kUserDictPath.c_str(),
      kIdfPath.c_str(), kStopWordPath.c_str());

  setupIO();
  std::string punctuations = R"( ;:,.!?-…()\"“”)";
  for (auto p : punctuations) {
      _punc_set.insert(p);
  }
}

int Tts::load_voices(const std::vector<std::string> &speaker_names,
                     std::vector<int64_t> &dims,
                     const std::string &voices_bin) {
  int n_speaker = speaker_names.size();
  int max_len = _style_dims[0]; // 510
  int emb_dim = _style_dims[2]; // 256

  std::ifstream file(voices_bin, std::ios::binary);
  if (!file) {
    std::cout << "fail to open " << voices_bin << std::endl;
    return -1;
  }

  file.seekg(0, std::ios::end);
  size_t file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::cout << voices_bin << " file_size:" << file_size <<std::endl;
  if (n_speaker * max_len * emb_dim * sizeof(float) != file_size) {
    std::cout << voices_bin << " file_size error, pleack check" << std::endl;
    return -2;
  }

  // 2. 读取数据到 uint8 缓冲区
  std::vector<uint8_t> buffer(file_size);
  file.read(reinterpret_cast<char *>(buffer.data()), file_size);
  file.close();

  // // 3. 重新解释为 float32 数组
  const float *float_data = reinterpret_cast<const float *>(buffer.data());
  for (int n = 0; n < n_speaker; ++n) {
    int chunk_size = max_len * emb_dim;
    _voices[speaker_names[n]].assign(float_data + n * chunk_size,
                                     float_data + n * chunk_size + chunk_size);
  }
  return 0;
}

void Tts::load_lexicons(const std::vector<std::string> &lexicon_files) {
  for (auto &fin : lexicon_files) {
    std::ifstream input(fin);

    std::string line;
    while (std::getline(input, line)) {
      auto arr = split_string(line, ' ');
      std::vector<std::string> tokens (arr.begin() + 1, arr.end());
      _word2token[arr[0]] = tokens;
    }
  }
  if (_word2token.size() > 0) {
    std::cout << "token2id size: " << _word2token.size();
  } else {
    std::cout << "token2id size: " << _word2token.size();
  }
}

void Tts::load_tokens(const std::string &token_file) {
  std::ifstream input(token_file);

  std::string line;
  while (std::getline(input, line)) {
    auto arr = split_string(line, ' ');
    if (arr.size() == 2) {
      _token2id[arr[0]] = stoi(arr[1]);
    } else {
      _token2id[" "] = stoi(arr[2]);
    }
  }
  if (_token2id.size() > 0) {
    std::cout  << "token2id size: " << _token2id.size();
  } else {
    std::cout << "token2id size: " << _token2id.size();
  }
}

std::vector<std::string> Tts::split_ch_eng(const std::string &text) {

    std::vector<std::string> ret;
    std::string cur;
    int cur_len = -1;
    for (size_t i = 0, len = 0; i < text.length(); i += len) {
        unsigned char byte = (unsigned)text[i];
        if (byte >= 0xFC) // lenght 6
            len = 6;
        else if (byte >= 0xF8)
            len = 5;
        else if (byte >= 0xF0)
            len = 4;
        else if (byte >= 0xE0)
            len = 3;
        else if (byte >= 0xC0)
            len = 2;
        else
            len = 1;
        auto sub = text.substr(i, len);
        
        bool is_punc  = _punc_set.count(text[i]);
        int tmp_len = is_punc ? 0 :len;

        if (cur_len != -1 and tmp_len != cur_len) {
            if (cur_len == 1) {
                std::transform(cur.begin(), cur.end(), cur.begin(),
                        [](unsigned char c) { return std::tolower(c); });
            }
            if (cur != " ") {
                ret.push_back(cur);
            }
            cur = "";
        }
        cur += sub;
        cur_len = tmp_len;
    }
    if (cur.size() > 0) {
        if (cur != " ") {
            ret.push_back(cur);
        }
    }
    for (const auto& p : ret) {
        std::cout << "convert:" << p << std::endl;
    }
    return ret;
}

void Tts::infer(std::vector<int64_t>& tokenids, 
                std::vector<float>& style,
                float speed) {

  /*
  auto tmp = load_vec("/home/nanxun/Documents/sherpa-onnx/scripts/kokoro/v1.1-zh/tokenids.txt");
  for (auto t :vec) {
    tokenids.push_back(int64_t(t));
  }
  style  = load_vec("/home/nanxun/Documents/sherpa-onnx/scripts/kokoro/v1.1-zh/style.txt");
  */

  Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(
      OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
  int64_t dims[2]; dims[0] = 1; dims[1] = tokenids.size();
  auto token_ort = Ort::Value::CreateTensor<int64_t>(
      memory_info, tokenids.data(), tokenids.size(), dims, 2);

  auto style_ort = Ort::Value::CreateTensor<float>(
      memory_info, style.data() , style.size(),
      &_style_dims[1], 2);

  int64_t speed_dim[1] =  {1};
  auto speed_ort = Ort::Value::CreateTensor<float>(memory_info, &speed, 1, 
          speed_dim, 1);
                                                   

  std::vector<Ort::Value> ort_inputs;
  ort_inputs.emplace_back(std::move(token_ort));
  ort_inputs.emplace_back(std::move(style_ort));
  ort_inputs.emplace_back(std::move(speed_ort));

  std::vector<Ort::Value> output_tensors = session_->Run(
      Ort::RunOptions{nullptr}, input_names_.data(), ort_inputs.data(),
      ort_inputs.size(), output_names_.data(), output_names_.size());

  if (output_tensors.empty() || !output_tensors.front().IsTensor()) {
      throw std::runtime_error("Invalid output tensors");
  }

  auto info = output_tensors.front().GetTensorTypeAndShapeInfo();
  std::vector<int64_t> shape = info.GetShape();
  std::cout << "shape: ";
  for (auto s :shape) {
      std::cout << s << " ";
  }
  std::cout <<std::endl;

  float *logits_data = output_tensors.front().GetTensorMutableData<float>();
  size_t element_count = info.GetElementCount();
  sherpa_onnx::WriteWave(std::string("out.wav"), _sample_rate, logits_data, element_count);
}

void Tts::run(const std::string &text, const std::string &voice) {
    std::vector<std::string> parts = split_ch_eng(text);

    std::vector<std::string> tokens;
    for (const auto& sent: parts) {
        unsigned char byte = (unsigned)sent[0]; 
        if (_punc_set.count(sent[0])) {
            std::cout << "add token:" <<sent<<std::endl;
            for (auto s :sent)  {
                std::string tmp(1, s);
                tokens.push_back(tmp);
                //tokens.push_back(" ");
            }
        } else if (byte < 0xC0) {  // eng
            if (_word2token.count(sent)) {
                std::cout << "add token:" <<sent<<std::endl;
                tokens.insert(tokens.end(), _word2token[sent].begin(), _word2token[sent].end());
            } else {
                std::cout << "skip eng:" <<  sent << std::endl;
            }
        } else  {
            std::vector<std::string> out;
            _jieba->Cut(sent, out);
            for (auto& o: out) {
                if (_word2token.count(o)) {
                    std::cout << "add token:" <<o<<std::endl;
                    tokens.insert(tokens.end(), _word2token[o].begin(), _word2token[o].end());
                } else {
                    // split into single hanzi
                    for (auto hanzi : utf8_to_charset(o))  {
                        std::cout << "add token:" <<hanzi<<std::endl;
                        if (_word2token.count(hanzi)) {
                            tokens.insert(tokens.end(), _word2token[hanzi].begin(), _word2token[hanzi].end());
                        }  else {
                            std::cout << "skip ch:" <<  sent << std::endl;
                        }
                    }
                }
            }
        }
    }

    std::vector<int64_t> token_ids;
    token_ids.push_back(0);
    for (auto& str : tokens) {
        token_ids.push_back(_token2id[str]);
        std::cout << "token_ids:" << str << " " << _token2id[str] << std::endl;
    }
    if (token_ids.size() > _max_len) {
        token_ids.resize(_max_len);
    }
    token_ids.push_back(0);

    std::vector<float> style;
    int64_t emb_dim = _style_dims[2]; 
    style.assign(_voices[voice].begin() + emb_dim * token_ids.size(), _voices[voice].begin() + emb_dim *token_ids.size() + emb_dim);

    infer(token_ids , style, 1.0);
}

void Tts::setupIO() {
  Ort::AllocatorWithDefaultOptions allocator;

  // 获取输入信息
  size_t num_input_nodes = session_->GetInputCount();
  input_names_.reserve(num_input_nodes);

  for (size_t i = 0; i < num_input_nodes; i++) {
    auto input_name = session_->GetInputNameAllocated(i, allocator);

    char *dest = new char[strlen(input_name.get()) + 1]; // +1 用于空终止符
    input_names_.push_back(dest);
    strcpy(dest, input_name.get());

    Ort::TypeInfo type_info = session_->GetInputTypeInfo(i);
    auto tensor_info = type_info.GetTensorTypeAndShapeInfo();

    std::vector<int64_t> input_dims = tensor_info.GetShape();
    std::cout << "Input " << i << " name: " << dest << std::endl;
    std::cout << "Input shape: ";
    for (auto dim : input_dims) {
      std::cout << dim << " ";
    }
    input_dims_.push_back(input_dims);
    std::cout << std::endl;
  }

  // 获取输出信息
  size_t num_output_nodes = session_->GetOutputCount();
  output_names_.reserve(num_output_nodes);

  for (size_t i = 0; i < num_output_nodes; i++) {
    auto output_name = session_->GetOutputNameAllocated(i, allocator);
    char *dest = new char[strlen(output_name.get()) + 1];
    strcpy(dest, output_name.get());
    output_names_.push_back(dest);

    Ort::TypeInfo type_info = session_->GetOutputTypeInfo(i);
    auto tensor_info = type_info.GetTensorTypeAndShapeInfo();

    std::vector<int64_t> output_dims = tensor_info.GetShape();
    std::cout << "Output " << i << " name: " << dest << std::endl;
    std::cout << "Output shape: ";
    for (auto dim : output_dims) {
      std::cout << dim << " ";
    }
    std::cout << std::endl;
  }
}

void Tts::getCustomMetadataMap(std::map<std::string, std::string> &data) {
  Ort::AllocatorWithDefaultOptions allocator;
  Ort::ModelMetadata model_metadata = session_->GetModelMetadata();

  // 获取自定义元数据数量
  auto keys = model_metadata.GetCustomMetadataMapKeysAllocated(allocator);
  std::cout << "\nCustom Metadata (" << keys.size() << " items):" << std::endl;

  // 遍历所有自定义元数据
  for (size_t i = 0; i < keys.size(); ++i) {
    const char *key = keys[i].get();
    auto value =
        model_metadata.LookupCustomMetadataMapAllocated(key, allocator);
    std::cout << key << ":" << value.get() << std::endl;
    data[std::string(key)] = std::string(value.get());
  }
}
