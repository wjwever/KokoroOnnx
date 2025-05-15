#include "kokoro.h"

int main() {
    try {
        std::string model_dir = "./model/";
        std::string jieba_dir = "./dict/";

        std::string kokoro_onnx = model_dir + "/kokoro.onnx";
        std::string tokens = model_dir + "/tokens.txt";
        std::vector<std::string> lexicons = {model_dir + "/lexicon-us-en.txt", model_dir + "/lexicon-zh.txt"};
        std::string voice_bin = model_dir + "/voices.bin";

        Tts tts(kokoro_onnx, tokens, lexicons, voice_bin, jieba_dir);
        tts.run("来听一听, 这个是什么口音? How are you doing? Are you ok? Thank you! 你觉得中英文说得如何呢?", "zf_001");
    }
    catch(std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
